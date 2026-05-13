#pragma once
#include <functional>
#include <string>

class AIAdvisor {
public:
    explicit AIAdvisor(const std::string& modelPath, int nCtx = 2048);
    ~AIAdvisor();

    AIAdvisor(const AIAdvisor&)            = delete;
    AIAdvisor& operator=(const AIAdvisor&) = delete;

    bool isLoaded() const { return m_loaded; }

    std::string query(const std::string& expenseSummary,
                      std::function<void(const std::string&)> tokenCallback = nullptr);

    void setMaxTokens(int n)     { m_maxTokens = n; }
    void setTemperature(float t) { m_temperature = t; }

private:
    std::string buildPrompt(const std::string& expenseSummary) const;

    struct LlamaState;
    LlamaState* m_state       = nullptr;
    bool        m_loaded      = false;
    int         m_maxTokens   = 512;
    float       m_temperature = 0.7f;
};
