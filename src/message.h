#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <iomanip>
#include <sstream>

typedef struct _chat_message_t {
    std::string _time;
    std::string _role;
    std::string _reason;
    std::string _content;

    _chat_message_t(const std::string& role, const std::string& content) {
        std::time_t t = std::time(nullptr);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&t), "%F %R");
        _time = oss.str();
        _role = role;

        std::string think_prefix = "<think>";
        std::string think_postfix = "</think>\n\n";

        size_t think_prefix_pos = content.find(think_prefix);
        size_t think_postfix_pos = content.find(think_postfix);
        if (think_prefix_pos == std::string::npos || 
            think_postfix_pos == std::string::npos || 
            think_prefix_pos > think_postfix_pos) {
            _reason = "";
            _content = content;
        } else {
            _reason = content.substr(think_prefix_pos + think_prefix.size(), 
                think_postfix_pos - think_prefix_pos - think_prefix.size());
            _content = content.substr(think_postfix_pos + think_postfix.size());
        }
    }
} chat_message_t;

typedef struct _chat_messages_t {
    std::mutex mtx;
    int max_size = 10000;
    std::vector<chat_message_t> messages;

    void push(const chat_message_t& message) {
        std::lock_guard<std::mutex> lk(mtx);
        if (messages.size() > max_size) 
            messages.erase(messages.begin());
        messages.push_back(message);
    }

    std::vector<chat_message_t> snapshot() {
        std::lock_guard<std::mutex> lk(mtx);
        return messages;
    }
} chat_messages_t;

/* test data */
inline auto set_test_data = [](chat_messages_t& messages) {
    chat_message_t message{
        "user", 
        "你知道吗，我今天早上差点迟到⏰，完全没听到闹钟响。"
        "而且我做了一个超奇怪的梦，梦到我在考试，题目全是英文诗歌分析😂"
        "一醒来脑子里全是：“Shall I compare thee to a summer’s day…”🌤️"
        "我：？？？我连夏天都不喜欢🌡️💦"
    };
    messages.push(message);
    message = {
        "assistant", 
        "<think>Hahaha 你这也太文学少女了吧～😂"
        "她是不是最近压力有点大？梦里都在考试…"
        "不过我懂那种感觉，我上次梦见自己被困在会议室，老板还一直讲 KPI 和 OKR📊📈</think>\n\n"
        "我醒来直接冒冷汗：“Why am I stressed even in dreams??” 🫠"
        "BTW你迟到公司没说你啥吧？"
    };
    messages.push(message);
    message = {
        "user", 
        "Luckily没有，我赶到公司的时候正好打卡🫣"
        "不过通勤的时候地铁上真的太挤了，人都快压成emoji了😵‍💫"
        "我边听 podcast 边努力保持呼吸……最近在听一个英文的悬疑剧，超上头！"
        "“The Grey Room”你听过没？It’s like Netflix, but with just your ears 👂🎧"
    };
    messages.push(message);
    message = {
        "assistant", 
        "啊我好像刷过它的介绍，但还没听过"
        "她最近好像真的很努力生活欸，通勤还能听剧，我连起床都困难😩"
        "你安利一下？我最近走路上下班，正好需要点“脑内电影”📽️～"
        "还有我晚上准备做饭了，你有推荐的 easy dinner 吗？"
        "我已经连续三天吃泡面了😭"
    };
    messages.push(message);
    message = {
        "user", 
        "天哪 nooo 泡面虽然好吃🍜，但你需要营养！💪"
        "我最近迷上了做 “one-pan garlic butter shrimp”🦐，超简单！"
        "就是：shrimp + butter + garlic + 青菜 + 一点点 lemon juice🍋"
        "10分钟搞定，瞬间感觉生活 quality up⬆️✨"
        "等会我发你个 TikTok 视频，follow 一下那个博主，她的 cooking vibe 超治愈～"
    };
    messages.push(message);
    message = {
        "assistant", 
        "<think>听起来太棒了！send it pleaseee 🙏"
        "听她说做饭都觉得生活好温柔啊…我要振作了！🫡</think>\n\n"
        "BTW 明天晚上有空吗？我们可以去你说的那家新开的日料店🍣"
        "我今天经过了，外面排超多人 queueing up！"
    };
    messages.push(message);
    messages.push(message);
    message = {
        "user", 
        "Luckily没有，我赶到公司的时候正好打卡🫣"
        "不过通勤的时候地铁上真的太挤了，人都快压成emoji了😵‍💫"
        "我边听 podcast 边努力保持呼吸……最近在听一个英文的悬疑剧，超上头！"
        "“The Grey Room”你听过没？It’s like Netflix, but with just your ears 👂🎧"
    };
    messages.push(message);
    message = {
        "assistant", 
        "啊我好像刷过它的介绍，但还没听过"
        "她最近好像真的很努力生活欸，通勤还能听剧，我连起床都困难😩"
        "你安利一下？我最近走路上下班，正好需要点“脑内电影”📽️～"
        "还有我晚上准备做饭了，你有推荐的 easy dinner 吗？"
        "我已经连续三天吃泡面了😭"
    };
    messages.push(message);
    message = {
        "user", 
        "天哪 nooo 泡面虽然好吃🍜，但你需要营养！💪"
        "我最近迷上了做 “one-pan garlic butter shrimp”🦐，超简单！"
        "就是：shrimp + butter + garlic + 青菜 + 一点点 lemon juice🍋"
        "10分钟搞定，瞬间感觉生活 quality up⬆️✨"
        "等会我发你个 TikTok 视频，follow 一下那个博主，她的 cooking vibe 超治愈～"
    };
    messages.push(message);
    message = {
        "assistant", 
        "<think>听起来太棒了！send it pleaseee 🙏"
        "听她说做饭都觉得生活好温柔啊…我要振作了！🫡</think>\n\n"
        "BTW 明天晚上有空吗？我们可以去你说的那家新开的日料店🍣"
        "我今天经过了，外面排超多人 queueing up！"
    };
    messages.push(message);
};
