#ifndef APPLE_SEQUENCER_HPP
#define APPLE_SEQUENCER_HPP

#include <random>
#include <vector>
#include <string>

/**
 * @file appleSequencer.hpp
 * @brief 服务端权威苹果序列生成器
 * 
 * 使用确定性随机种子生成苹果序列，确保双方玩家看到相同的苹果。
 * 支持难度递增：下落速度、生成间隔、字母范围、同屏苹果数。
 */

struct AppleSpawnInfo
{
    char letter;        // 苹果字母
    float x;            // 归一化 X 位置 (0.0~1.0)
    int64_t spawnTime;  // 生成时间戳 (ms)
    int fallSpeed;      // 下落速度 (像素/帧)
};

class AppleSequencer
{
public:
    AppleSequencer();

    /**
     * @brief 初始化序列生成器
     * @param seed 随机种子，双方使用相同种子确保一致性
     * @param level 起始关卡
     */
    void initialize(uint32_t seed, int level = 1);

    /**
     * @brief 生成下一个苹果
     * @param currentTimeMs 当前游戏时间 (ms)
     * @return 苹果生成信息
     */
    AppleSpawnInfo nextApple(int64_t currentTimeMs);

    /**
     * @brief 设置关卡（难度递增）
     * @param level 新关卡
     */
    void setLevel(int level);

    /**
     * @brief 设置下落速度
     * @param speed 下落速度 (像素/帧)
     */
    void setFallSpeed(int speed);

    /**
     * @brief 设置生成间隔
     * @param interval 生成间隔 (ms)
     */
    void setSpawnInterval(int interval);

    /**
     * @brief 设置最大同屏苹果数
     * @param max 最大同屏苹果数
     */
    void setMaxActiveApples(int max);

    int fallSpeed() const { return m_fallSpeed; }
    int spawnInterval() const { return m_spawnInterval; }
    int maxActiveApples() const { return m_maxActiveApples; }
    int level() const { return m_level; }

private:
    /**
     * @brief 根据关卡确定可用字母范围
     * @return 可用字母列表
     */
    std::vector<char> getAvailableLetters() const;

    /**
     * @brief 生成不重叠的 X 位置
     * @param activeXs 当前活跃苹果的 X 位置列表
     * @return 归一化 X 位置
     */
    float generateX(const std::vector<float>& activeXs);

    uint32_t m_seed;
    int m_level;
    int m_fallSpeed;
    int m_spawnInterval;
    int m_maxActiveApples;
    std::mt19937 m_rng;
    int m_appleCount;  // 已生成的苹果计数，用于避免重复位置
};

#endif // APPLE_SEQUENCER_HPP