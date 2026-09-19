#pragma once

#include <string>
#include "vcsdk/core/Command.hpp"

namespace vcsdk::security {

// Mode de fonctionnement du SDK (section 19). READ_ONLY est le mode
// PAR DEFAUT impose par la specification -- ne jamais changer cette
// valeur par defaut sans une decision explicite et documentee.
enum class SdkMode { READ_ONLY, FULL };

// Valide chaque commande avant emission (section 19).
//
// *** NUANCE IMPORTANTE PAR RAPPORT A LA SECTION 8 ***
// La section 8 interdit a DEV 1 d'interpreter le contenu d'une commande.
// La section 19 exige pourtant que le SDK sache reconnaitre les
// commandes "potentiellement modificatrices" pour les bloquer par
// defaut. Ce n'est pas une contradiction : c'est une exception
// deliberee et limitee, strictement pour la securite, pas pour la
// logique metier. Le SDK reste ignorant du SENS des commandes de
// lecture -- il reconnait seulement une liste fermee de motifs connus
// comme dangereux (deny-list), sans jamais decoder une reponse.
//
// Portee actuelle : les commandes textuelles de type ELM327/OBD-II
// ("010C", "04"...). Les trames CAN brutes (convention "ID#DATA") ne
// sont PAS classifiees ici -- leur interpretation necessiterait un
// fichier DBC, hors perimetre du SDK (section 34).
class CommandSecurityPolicy {
public:
    explicit CommandSecurityPolicy(SdkMode mode = SdkMode::READ_ONLY) : mode_(mode) {}

    // Leve ConnectorError(PERMISSION_DENIED) si la commande est bloquee
    // par la politique courante. Ne fait rien (retour normal) sinon.
    void validate(const vcsdk::core::Command& command) const;

    SdkMode mode() const { return mode_; }
    void setMode(SdkMode mode) { mode_ = mode; }

    // Expose pour les tests et pour la transparence (ex: interface de
    // debug) -- indique si un payload est reconnu comme modificateur,
    // independamment du mode courant.
    static bool isKnownModifyingCommand(const std::string& payload);

private:
    SdkMode mode_;
};

}  // namespace vcsdk::security
