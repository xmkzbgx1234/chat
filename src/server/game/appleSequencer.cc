#include "server/game/appleSequencer.hpp"

#include <algorithm>
#include <cmath>
#include <random>

AppleSequencer::AppleSequencer()
    : m_seed(0)
    , m_level(1)
    , m_fallSpeed(2)
    , m_spawnInterval(2000)
    , m_maxActiveApples(5)
    , m_rng()
    , m_appleCount(0)
{
}

void AppleSequencer::initialize(uint32_t seed, int level)
{
    m_seed       = seed;
    m_appleCount = 0;
    m_rng.seed(seed);
    setLevel(level);
}

AppleSpawnInfo AppleSequencer::nextApple(int64_t currentTimeMs)
{
    auto const letters = getAvailableLetters();
    std::uniform_int_distribution<std::size_t> letterDist(0, letters.size() - 1);
    char const letter = letters[letterDist(m_rng)];

    // Generate a random X; overlap avoidance is done by the caller
    // via generateX() when active positions are known.
    float const x = generateX({});

    AppleSpawnInfo info;
    info.letter    = letter;
    info.x         = x;
    info.spawnTime = currentTimeMs;
    info.fallSpeed = m_fallSpeed;

    ++m_appleCount;
    return info;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::vector<char> AppleSequencer::getAvailableLetters() const
{
    if (m_level <= 2)
    {
        return {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'};
    }
    if (m_level <= 4)
    {
        return {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'};
    }
    // Level 5+
    return {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
            'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
            'u', 'v', 'w', 'x', 'y', 'z'};
}

float AppleSequencer::generateX(const std::vector<float>& activeXs)
{
    std::uniform_real_distribution<float> dist(0.1f, 0.9f);

    for (int attempt = 0; attempt < 10; ++attempt)
    {
        float const x = dist(m_rng);

        bool overlap = false;
        for (float ax : activeXs)
        {
            if (std::fabs(x - ax) < 0.1f)
            {
                overlap = true;
                break;
            }
        }

        if (!overlap)
        {
            return x;
        }
    }

    // Fallback – return a random X even if overlap could not be avoided
    return dist(m_rng);
}

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------

void AppleSequencer::setLevel(int level)
{
    m_level = level;

    // Difficulty scaling with level
    m_fallSpeed       = std::min(8, 2 + (level - 1) * 1);         // px/frame
    m_spawnInterval   = std::max(400, 2000 - (level - 1) * 150);  // ms
    m_maxActiveApples = std::min(15, 5 + (level - 1) / 2);        // apples
}

void AppleSequencer::setFallSpeed(int speed)
{
    m_fallSpeed = speed;
}

void AppleSequencer::setSpawnInterval(int interval)
{
    m_spawnInterval = interval;
}

void AppleSequencer::setMaxActiveApples(int max)
{
    m_maxActiveApples = max;
}
