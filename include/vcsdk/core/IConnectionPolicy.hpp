#pragma once

#include <chrono>

namespace vcsdk::core {

// Interface listee explicitement en section 21 de la specification.
// Permet d'echanger la strategie de reconnexion sans toucher aux
// connecteurs qui l'utilisent (pattern Strategy, section 22).
class IConnectionPolicy {
public:
    virtual ~IConnectionPolicy() = default;

    // Doit-on retenter une reconnexion apres 'attemptNumber' tentatives
    // deja effectuees (0 = aucune tentative encore faite) ?
    virtual bool shouldReconnect(int attemptNumber) const = 0;

    // Delai a attendre avant la tentative numero 'attemptNumber' (1-based).
    virtual std::chrono::milliseconds getNextRetryDelay(int attemptNumber) const = 0;

    virtual int getMaxAttempts() const = 0;
};

}  // namespace vcsdk::core
