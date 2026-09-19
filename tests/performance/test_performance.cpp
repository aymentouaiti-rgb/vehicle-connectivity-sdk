// Tests de performance (section 26). Mesure la latence et le taux de
// perte de messages a differents debits, sur une session longue.
//
// Ces tests utilisent le VirtualConnector (le plus valide) contre le
// simulateur de test. Ce ne sont pas des benchmarks Google Benchmark
// (non installes dans cet environnement) mais des mesures directes,
// suffisantes pour caracteriser le comportement du SDK a differentes
// charges, avec des seuils explicites et documentes.

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include <numeric>
#include "vcsdk/connectors/virtual/VirtualConnector.hpp"

using namespace vcsdk::core;
using namespace vcsdk::connectors;

namespace {
constexpr int kTestPort = 35020;

// Envoie 'count' commandes au debit demande et retourne les latences
// mesurees (en ms) pour chaque reponse recue avec succes.
std::vector<int> runAtRate(VirtualConnector& connector, int count, int perSecond) {
    std::vector<int> latencies;
    auto interval = std::chrono::milliseconds(perSecond > 0 ? (1000 / perSecond) : 0);

    for (int i = 0; i < count; ++i) {
        auto start = std::chrono::steady_clock::now();
        Command cmd;
        cmd.commandId = "CMD-" + std::to_string(i);
        cmd.payload = "010C";
        try {
            connector.send(cmd);
            RawVehicleMessage msg = connector.receive();
            latencies.push_back(msg.latencyMs);
        } catch (const std::exception&) {
            // Rate limit atteint ou autre erreur : comptabilise comme perte,
            // pas comme un crash du test.
        }
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed < interval) {
            std::this_thread::sleep_for(interval - elapsed);
        }
    }
    return latencies;
}
}  // namespace

// Section 26 : "1 message/s"
TEST(PerformanceTest, Debit1MessageParSeconde) {
    VirtualConnector connector("127.0.0.1", kTestPort);
    connector.setMaxCommandRate(50);  // pas de limitation a ce debit
    connector.connect();

    auto latencies = runAtRate(connector, 5, 1);
    EXPECT_EQ(latencies.size(), 5u);  // aucune perte attendue a ce debit

    connector.disconnect();
}

// Section 26 : "10 messages/s"
TEST(PerformanceTest, Debit10MessagesParSeconde) {
    VirtualConnector connector("127.0.0.1", kTestPort);
    connector.setMaxCommandRate(50);
    connector.connect();

    auto latencies = runAtRate(connector, 20, 10);
    double lossRate = 1.0 - (static_cast<double>(latencies.size()) / 20.0);
    EXPECT_LT(lossRate, 0.05);  // moins de 5% de perte attendue

    connector.disconnect();
}

// Section 26 : "50 messages/s"
TEST(PerformanceTest, Debit50MessagesParSeconde) {
    VirtualConnector connector("127.0.0.1", kTestPort);
    connector.setMaxCommandRate(60);  // legerement au-dessus pour ne pas s'auto-bloquer
    connector.connect();

    auto latencies = runAtRate(connector, 50, 50);
    double lossRate = 1.0 - (static_cast<double>(latencies.size()) / 50.0);
    EXPECT_LT(lossRate, 0.10);

    if (!latencies.empty()) {
        double avg = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        EXPECT_LT(avg, 100.0);  // latence moyenne raisonnable sur simulateur local
    }

    connector.disconnect();
}

// Section 26 : "100 messages/s" -- au-dela de la limite par defaut (20/s),
// ce test verifie explicitement que le RateLimiter protege le systeme
// au lieu de le laisser en absorber plus qu'il ne peut, plutot que
// d'exiger un debit que le SDK n'est pas cense soutenir sans reconfiguration.
TEST(PerformanceTest, Debit100MessagesParSecondeAvecLimitationActive) {
    VirtualConnector connector("127.0.0.1", kTestPort);
    // Limite volontairement laissee a 20/s (valeur par defaut, section 15)
    connector.connect();

    int successCount = 0;
    int rejectedCount = 0;
    for (int i = 0; i < 100; ++i) {
        Command cmd;
        cmd.commandId = "CMD-" + std::to_string(i);
        cmd.payload = "010C";
        try {
            connector.send(cmd);
            connector.receive();
            successCount++;
        } catch (const ConnectorError&) {
            rejectedCount++;
        }
    }
    // Preuve que le rate limiter a bien agi : pas 100 succes d'un coup.
    EXPECT_LT(successCount, 100);
    EXPECT_GT(rejectedCount, 0);

    connector.disconnect();
}

// Section 26 : "sessions longues" -- verifie l'absence de degradation
// notable de latence sur un plus grand nombre de messages consecutifs.
TEST(PerformanceTest, SessionLongueSansDegradationNotable) {
    VirtualConnector connector("127.0.0.1", kTestPort);
    connector.setMaxCommandRate(100);
    connector.connect();

    auto latencies = runAtRate(connector, 200, 40);
    ASSERT_FALSE(latencies.empty());

    size_t half = latencies.size() / 2;
    double firstHalfAvg = std::accumulate(latencies.begin(), latencies.begin() + half, 0.0) / half;
    double secondHalfAvg = std::accumulate(latencies.begin() + half, latencies.end(), 0.0) /
                            (latencies.size() - half);

    // La seconde moitie ne doit pas etre significativement plus lente
    // que la premiere (pas de fuite/degradation progressive).
    EXPECT_LT(secondHalfAvg, firstHalfAvg + 50.0);

    connector.disconnect();
}

// Section 26 : "reconnexions repetees" -- verifie que plusieurs cycles
// de coupure/reconnexion successifs restent stables.
TEST(PerformanceTest, ReconnexionsRepeteesRestentStables) {
    VirtualConnector connector("127.0.0.1", kTestPort);
    connector.connect();

    int reconnectSuccesses = 0;
    for (int i = 0; i < 3; ++i) {
        Command drop;
        drop.commandId = "CMD-DROP-" + std::to_string(i);
        drop.payload = "DROPLINK";
        connector.send(drop);
        try {
            connector.receive();
        } catch (const ConnectorError&) {
            if (connector.getStatus() == ConnectionState::CONNECTED) {
                reconnectSuccesses++;
            }
        }
    }
    EXPECT_EQ(reconnectSuccesses, 3);  // les 3 cycles doivent reussir

    connector.disconnect();
}
