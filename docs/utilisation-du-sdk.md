# Guide — Utilisation du SDK

## Utilisation minimale (sans configuration)

```cpp
#include "vcsdk/connectors/virtual/VirtualConnector.hpp"
#include "vcsdk/core/VehicleSession.hpp"

using namespace vcsdk::core;
using namespace vcsdk::connectors;

auto connector = std::make_shared<VirtualConnector>("127.0.0.1", 35000);
VehicleSession session("SESSION-001", connector, "VIRTUAL");

session.start();

Command cmd;
cmd.commandId = "CMD-1";
cmd.payload = "010C";  // regime moteur
RawVehicleMessage msg = session.send(cmd);

std::cout << msg.rawPayload << std::endl;  // "410C1AF8"

session.stop();
```

## Utilisation recommandée (via configuration + Factory)

```cpp
#include "vcsdk/configuration/Config.hpp"
#include "vcsdk/factory/ConnectorFactory.hpp"
#include "vcsdk/core/VehicleSession.hpp"

using namespace vcsdk;

auto config = configuration::loadFromYamlFile("config.yaml");
auto connector = factory::ConnectorFactory::create(config);

core::VehicleSession session("SESSION-001", connector, config.connectorType);
session.start();
// ...
session.stop();
```

C'est la méthode à privilégier : elle permet de changer de connecteur
(Virtual, TCP, Serial, Bluetooth, SocketCAN) **sans recompiler**, juste
en modifiant le fichier `config.yaml`.

## Gérer les erreurs

Toutes les erreurs du SDK héritent de `ConnectorError` :

```cpp
try {
    session.send(cmd);
} catch (const core::ConnectorError& e) {
    std::cerr << "Erreur [" << e.code() << "] : " << e.message() << std::endl;
    // e.code() vaut par ex. ErrorCode::TIMEOUT, ErrorCode::CONNECTION_LOST...
}
```

## Consulter les métriques

```cpp
const auto& stats = session.getStatistics();
std::cout << "Commandes envoyees : " << stats.commandsSent << std::endl;
std::cout << "Reponses recues    : " << stats.messagesReceived << std::endl;
```

Pour les métriques détaillées au niveau du connecteur (si celui-ci les
expose, comme `VirtualConnector`) :

```cpp
auto virtualConn = std::dynamic_pointer_cast<VirtualConnector>(connector);
if (virtualConn) {
    std::cout << virtualConn->getMetrics().report();
}
```

## Passer en mode FULL (déconseillé sauf besoin explicite)

Par défaut, le SDK bloque les commandes potentiellement destructrices
(section 19). Pour les autoriser explicitement (ex: environnement de
test contrôlé) :

```cpp
virtualConn->setSecurityMode(vcsdk::security::SdkMode::FULL);
```

**Ne jamais faire ceci dans un contexte de production sans validation
explicite préalable.**

## Écrire un nouveau connecteur

Voir `docs/creer-un-connecteur.md`.
