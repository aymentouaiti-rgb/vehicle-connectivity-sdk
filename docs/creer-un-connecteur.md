# Guide — Créer un nouveau connecteur

Ce guide explique comment ajouter un nouveau type de connecteur au SDK
**sans modifier une seule ligne du cœur** (`core/`) — c'est le critère
d'acceptation le plus important de la spécification (section 33 :
*"un nouveau connecteur peut être ajouté sans modifier le Core"*).

## Étape 1 — Créer les dossiers

```
include/vcsdk/connectors/monconnecteur/MonConnecteur.hpp
src/connectors/monconnecteur/MonConnecteur.cpp
```

## Étape 2 — Hériter de IConnector

```cpp
#pragma once
#include "vcsdk/core/IConnector.hpp"
#include "vcsdk/security/CommandSecurityPolicy.hpp"
#include "vcsdk/retry/RetryPolicy.hpp"

namespace vcsdk::connectors {
using namespace vcsdk::core;

class MonConnecteur : public IConnector {
public:
    MonConnecteur(/* vos parametres de connexion */);
    ~MonConnecteur() override;

    void discover() override;
    void connect() override;
    void disconnect() override;
    void send(const Command& command) override;
    RawVehicleMessage receive() override;
    bool healthCheck() override;
    ConnectionState getStatus() const override;

private:
    ConnectionState state_ = ConnectionState::DISCONNECTED;
    vcsdk::security::CommandSecurityPolicy securityPolicy_;
    vcsdk::retry::RetryPolicy retryPolicy_;
};

}  // namespace vcsdk::connectors
```

## Étape 3 — Implémenter les 7 méthodes obligatoires

| Méthode | Ce qu'elle doit faire |
|---|---|
| discover() | Rechercher les équipements disponibles (ou ne rien faire si non applicable) |
| connect() | Établir la connexion, transitions d'état via setState() |
| disconnect() | Fermer proprement |
| send() | Toujours valider via securityPolicy_.validate(command) avant d'écrire |
| receive() | Retourner un RawVehicleMessage, jamais interpréter rawPayload |
| healthCheck() | Vérification technique légère, sans commande métier |
| getStatus() | Retourner l'état courant |

## Étape 4 — Brancher la reconnexion automatique (recommandé)

Dans receive(), en cas de perte de connexion détectée :

```cpp
vcsdk::retry::performReconnect(
    [this]() { /* votre logique de reconnexion */ },
    [this](ConnectionState s) { setState(s); },
    retryPolicy_, "MonConnecteur", nowIso8601());
```

## Étape 5 — Enregistrer dans ConnectorFactory (optionnel)

Si vous voulez que votre connecteur soit sélectionnable via la
configuration YAML :

```cpp
if (config.connectorType == "monconnecteur") {
    return std::make_shared<MonConnecteur>(/* ... */);
}
```

## Étape 6 — Ajouter au CMakeLists.txt

```cmake
add_library(vcsdk
    ...
    src/connectors/monconnecteur/MonConnecteur.cpp
)
```

## Étape 7 — Écrire les tests contractuels

Créez tests/contract/test_monconnecteur.cpp en suivant exactement le
même schéma que tests/contract/test_virtual_connector.cpp : connexion
normale, adaptateur absent, healthCheck, mode READ_ONLY.

## Ce qu'il ne faut JAMAIS faire

- Modifier core/IConnector.hpp pour votre cas particulier
- Ajouter un if (type == "monconnecteur") dans un autre connecteur
- Interpréter le contenu de command.payload au-delà de la classification
  de sécurité (section 19)
