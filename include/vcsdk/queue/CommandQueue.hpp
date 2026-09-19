#pragma once

#include <queue>
#include <vector>
#include <mutex>
#include <optional>
#include <algorithm>
#include "vcsdk/core/Command.hpp"
#include "vcsdk/core/ConnectorError.hpp"

namespace vcsdk::queue {

using vcsdk::core::Command;
using vcsdk::core::CommandPriority;

// File de commandes avec priorites (section 10). Objectifs explicites de
// la spec : eviter les requetes concurrentes incompatibles, conserver
// l'ordre, controler la charge, gerer les priorites.
class CommandQueue {
public:
    explicit CommandQueue(size_t maxSize = 1000) : maxSize_(maxSize) {}

    // Ajoute une commande. Leve BUFFER_OVERFLOW si la file est saturee
    // (section 15 : gestion du debit).
    void enqueue(Command command) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (items_.size() >= maxSize_) {
            throw vcsdk::core::ConnectorError(
                vcsdk::core::ErrorCode::BUFFER_OVERFLOW,
                "Command queue is full", vcsdk::core::Severity::ERROR,
                "CommandQueue", "", command.correlationId, "");
        }
        items_.push_back(std::move(command));
        // Priorite basse (Emergency=1) sortira en premier : tri stable
        // par priorite croissante, en preservant l'ordre d'arrivee (FIFO)
        // au sein d'une meme priorite.
        std::stable_sort(items_.begin(), items_.end(), [](const Command& a, const Command& b) {
            return static_cast<int>(a.priority) < static_cast<int>(b.priority);
        });
    }

    // Retire et retourne la commande la plus prioritaire. std::nullopt
    // si la file est vide (evite de lever une exception dans un cas normal).
    std::optional<Command> dequeue() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (items_.empty()) return std::nullopt;
        Command front = std::move(items_.front());
        items_.erase(items_.begin());
        return front;
    }

    // Annule une commande en attente par son id (section 21 : ICommandQueue.cancel()).
    bool cancel(const std::string& commandId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(items_.begin(), items_.end(),
            [&](const Command& c) { return c.commandId == commandId; });
        if (it == items_.end()) return false;
        items_.erase(it);
        return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.clear();
    }

    size_t getQueueSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.size();
    }

private:
    mutable std::mutex mutex_;
    std::vector<Command> items_;
    size_t maxSize_;
};

}  // namespace vcsdk::queue
