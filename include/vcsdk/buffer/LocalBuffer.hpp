#pragma once

#include <deque>
#include <mutex>
#include <optional>
#include "vcsdk/core/ConnectorError.hpp"

namespace vcsdk::buffer {

// Buffer technique generique (section 14). Conserve temporairement des
// reponses recues, messages non consommes ou evenements de connexion.
//
// *** IMPORTANT (section 14) *** : "Le SDK ne doit pas implementer un
// Data Lake ou stockage metier. Il doit uniquement disposer d'un buffer
// technique." -- pas de persistance disque, pas de requetage, juste une
// file bornee en memoire (FIFO), qui deborde proprement plutot que de
// grossir indefiniment.
template <typename T>
class LocalBuffer {
public:
    explicit LocalBuffer(size_t maxSize = 1000) : maxSize_(maxSize) {}

    // Leve BUFFER_OVERFLOW si le buffer est plein (section 15 : la
    // taille du buffer fait partie des limites de debit a controler).
    void push(T item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (items_.size() >= maxSize_) {
            throw vcsdk::core::ConnectorError(
                vcsdk::core::ErrorCode::BUFFER_OVERFLOW,
                "Local buffer is full", vcsdk::core::Severity::ERROR,
                "LocalBuffer", "", "", "");
        }
        items_.push_back(std::move(item));
    }

    std::optional<T> pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (items_.empty()) return std::nullopt;
        T front = std::move(items_.front());
        items_.pop_front();
        return front;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.empty();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        items_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::deque<T> items_;
    size_t maxSize_;
};

}  // namespace vcsdk::buffer
