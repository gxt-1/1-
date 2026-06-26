/* ============================================================
 *  键盘练习程序 v2.0 (Typing Trainer)
 *  开发平台: easyx 图形接口库 + Visual C++
 *  功能: 用户注册/登录、用户管理、成绩历史记录、时间/速度设置、
 *        个人按键习惯统计、打字特效显示
 *  采用面向对象设计思想
 *  完成人: 20250612132 郭祥通
 * ============================================================ */

#include <graphics.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <windows.h>
#include <math.h>

#define MAX_USERS      50
#define MAX_SCORES     200
#define MAX_HISTORY    100
#define USER_FILE      "users.dat"
#define SCORE_FILE     "scores.dat"
#define CONFIG_FILE    "config.dat"
#define KEYSTATS_FILE  "keystats.dat"
#define SCREEN_W       800
#define SCREEN_H       600

/* ============================================================
 *  枚举类型定义
 * ============================================================ */
typedef enum {
    PAGE_LOGIN, PAGE_REGISTER, PAGE_MAIN_MENU,
    PAGE_PRACTICE, PAGE_RANKING, PAGE_HISTORY,
    PAGE_SETTINGS, PAGE_PROFILE, PAGE_RESULT, PAGE_EXIT
} PageType;

typedef enum {
    DIFF_EASY, DIFF_NORMAL, DIFF_HARD
} Difficulty;

typedef enum {
    EFT_NONE, EFT_FLASH, EFT_PARTICLE
} EffectType;

/* ============================================================
 *  结构体: KeyStats —— 单个按键统计（个人按键习惯）
 * ============================================================ */
struct KeyStatData {
    int totalPress;     // 总按压次数
    int correctPress;   // 正确次数
    int wrongPress;     // 错误次数
    int totalTime;      // 总响应时间(ms)
    int avgTime;        // 平均响应时间(ms)
};

/* ============================================================
 *  类1: KeyStatsManager —— 按键习惯统计管理
 * ============================================================ */
class KeyStatsManager {
private:
    KeyStatData stats[128];  // ASCII 0-127
    char owner[32];
    bool loaded;

public:
    KeyStatsManager() {
        memset(stats, 0, sizeof(stats));
        owner[0] = '\0';
        loaded = false;
    }

    void setOwner(const char* username) {
        strcpy(owner, username);
        loadStats();
    }

    void loadStats() {
        if (owner[0] == '\0') return;
        char filename[64];
        sprintf(filename, "%s_%s", owner, KEYSTATS_FILE);
        FILE* fp = fopen(filename, "rb");
        if (fp != NULL) {
            fread(stats, sizeof(KeyStatData), 128, fp);
            fclose(fp);
            loaded = true;
        }
    }

    void saveStats() {
        if (owner[0] == '\0') return;
        char filename[64];
        sprintf(filename, "%s_%s", owner, KEYSTATS_FILE);
        FILE* fp = fopen(filename, "wb");
        if (fp != NULL) {
            fwrite(stats, sizeof(KeyStatData), 128, fp);
            fclose(fp);
        }
    }

    void recordPress(char ch, bool correct, int responseTime) {
        if (ch < 0 || ch >= 128) return;
        stats[(int)ch].totalPress++;
        if (correct) stats[(int)ch].correctPress++;
        else stats[(int)ch].wrongPress++;
        stats[(int)ch].totalTime += responseTime;
        if (stats[(int)ch].totalPress > 0)
            stats[(int)ch].avgTime = stats[(int)ch].totalTime / stats[(int)ch].totalPress;
    }

    KeyStatData* getStats(char ch) {
        if (ch < 0 || ch >= 128) return NULL;
        return &stats[(int)ch];
    }

    // 获取最容易出错的按键
    char getWeakestKey() {
        char worst = 'a';
        double worstRate = -1;
        for (int i = 32; i < 127; i++) {
            if (stats[i].totalPress >= 5) {
                double rate = (double)stats[i].wrongPress / stats[i].totalPress;
                if (rate > worstRate) {
                    worstRate = rate;
                    worst = (char)i;
                }
            }
        }
        return worst;
    }

    // 获取最熟练的按键
    char getBestKey() {
        char best = 'a';
        double bestRate = 2.0;
        for (int i = 32; i < 127; i++) {
            if (stats[i].totalPress >= 5) {
                double rate = (double)stats[i].wrongPress / stats[i].totalPress;
                if (rate < bestRate) {
                    bestRate = rate;
                    best = (char)i;
                }
            }
        }
        return best;
    }

    int getAccuracy(char ch) {
        if (ch < 0 || ch >= 128) return 0;
        if (stats[(int)ch].totalPress == 0) return 0;
        return (int)(stats[(int)ch].correctPress * 100.0 / stats[(int)ch].totalPress);
    }

    void clearStats() {
        memset(stats, 0, sizeof(stats));
        saveStats();
    }
};

/* ============================================================
 *  类2: User —— 用户实体类
 * ============================================================ */
class User {
public:
    int  id;
    char username[32];
    char password[32];
    char nickname[32];
    int  totalPractice;
    int  bestSpeed;
    int  registerTime;

    User() {
        id = 0;
        username[0] = '\0';
        password[0] = '\0';
        nickname[0] = '\0';
        totalPractice = 0;
        bestSpeed = 0;
        registerTime = 0;
    }

    void setInfo(int _id, const char* _user, const char* _pwd,
                 const char* _nick, int _regTime) {
        id = _id;
        strcpy(username, _user);
        strcpy(password, _pwd);
        strcpy(nickname, _nick);
        totalPractice = 0;
        bestSpeed = 0;
        registerTime = _regTime;
    }
};

/* ============================================================
 *  类3: UserManager —— 用户管理类
 * ============================================================ */
class UserManager {
private:
    User users[MAX_USERS];
    int  userCount;
    int  currentUserId;
    char currentUsername[32];

public:
    UserManager() {
        userCount = 0;
        currentUserId = -1;
        currentUsername[0] = '\0';
        loadUsers();
    }

    void loadUsers() {
        FILE* fp = fopen(USER_FILE, "rb");
        if (fp == NULL) return;
        userCount = fread(users, sizeof(User), MAX_USERS, fp);
        fclose(fp);
    }

    void saveUsers() {
        FILE* fp = fopen(USER_FILE, "wb");
        if (fp != NULL) {
            fwrite(users, sizeof(User), userCount, fp);
            fclose(fp);
        }
    }

    int registerUser(const char* username, const char* password,
                      const char* nickname) {
        if (userCount >= MAX_USERS) return -1;
        if (strlen(username) < 2 || strlen(password) < 2) return -3;
        for (int i = 0; i < userCount; i++) {
            if (strcmp(users[i].username, username) == 0)
                return -2;  // 用户名已存在
        }
        users[userCount].setInfo(userCount + 1, username, password,
                                  nickname, (int)time(NULL));
        userCount++;
        saveUsers();
        return 0;
    }

    int login(const char* username, const char* password) {
        for (int i = 0; i < userCount; i++) {
            if (strcmp(users[i].username, username) == 0 &&
                strcmp(users[i].password, password) == 0) {
                currentUserId = users[i].id;
                strcpy(currentUsername, users[i].username);
                return i;
            }
        }
        return -1;
    }

    void logout() {
        currentUserId = -1;
        currentUsername[0] = '\0';
    }

    int updateProfile(int userIdx, const char* newNickname,
                       const char* newPassword) {
        if (userIdx < 0 || userIdx >= userCount) return -1;
        if (newNickname[0] != '\0')
            strcpy(users[userIdx].nickname, newNickname);
        if (newPassword[0] != '\0' && strlen(newPassword) >= 2)
            strcpy(users[userIdx].password, newPassword);
        saveUsers();
        return 0;
    }

    int getCurrentUserIndex() {
        if (currentUserId < 0) return -1;
        for (int i = 0; i < userCount; i++)
            if (users[i].id == currentUserId) return i;
        return -1;
    }

    User* getUserByIndex(int idx) {
        if (idx < 0 || idx >= userCount) return NULL;
        return &users[idx];
    }

    int getUserCount() { return userCount; }
    const char* getCurrentUsername() { return currentUsername; }
    bool isLoggedIn() { return currentUserId >= 0; }
};

/* ============================================================
 *  类4: ScoreRecord —— 成绩记录实体类
 * ============================================================ */
class ScoreRecord {
public:
    char username[32];
    int  wpm;
    int  accuracy;
    int  duration;
    int  scoreDate;
    int  difficulty;
    int  totalChars;
    int  correctChars;

    ScoreRecord() {
        username[0] = '\0';
        wpm = 0;
        accuracy = 0;
        duration = 0;
        scoreDate = 0;
        difficulty = 0;
        totalChars = 0;
        correctChars = 0;
    }

    void setRecord(const char* _user, int _wpm, int _acc,
                   int _dur, int _diff, int _total, int _correct) {
        strcpy(username, _user);
        wpm = _wpm;
        accuracy = _acc;
        duration = _dur;
        scoreDate = (int)time(NULL);
        difficulty = _diff;
        totalChars = _total;
        correctChars = _correct;
    }
};

/* ============================================================
 *  类5: ScoreManager —— 成绩管理类
 * ============================================================ */
class ScoreManager {
private:
    ScoreRecord scores[MAX_SCORES];
    int scoreCount;

public:
    ScoreManager() {
        scoreCount = 0;
        loadScores();
    }

    void loadScores() {
        FILE* fp = fopen(SCORE_FILE, "rb");
        if (fp == NULL) return;
        scoreCount = fread(scores, sizeof(ScoreRecord), MAX_SCORES, fp);
        fclose(fp);
    }

    void saveScores() {
        FILE* fp = fopen(SCORE_FILE, "wb");
        if (fp != NULL) {
            fwrite(scores, sizeof(ScoreRecord), scoreCount, fp);
            fclose(fp);
        }
    }

    void addScore(const char* username, int wpm, int accuracy,
                  int duration, int difficulty, int totalChars, int correctChars) {
        if (scoreCount >= MAX_SCORES) {
            for (int i = 0; i < MAX_SCORES - 1; i++)
                scores[i] = scores[i + 1];
            scoreCount = MAX_SCORES - 1;
        }
        scores[scoreCount].setRecord(username, wpm, accuracy,
                                      duration, difficulty, totalChars, correctChars);
        scoreCount++;
        saveScores();
    }

    void getRanking(ScoreRecord* out, int& outCount, int topN) {
        outCount = (scoreCount < topN) ? scoreCount : topN;
        ScoreRecord temp[MAX_SCORES];
        for (int i = 0; i < scoreCount; i++) temp[i] = scores[i];
        // 按 WPM 降序排序（冒泡排序）
        for (int i = 0; i < scoreCount - 1; i++) {
            for (int j = 0; j < scoreCount - 1 - i; j++) {
                if (temp[j].wpm < temp[j + 1].wpm) {
                    ScoreRecord t = temp[j];
                    temp[j] = temp[j + 1];
                    temp[j + 1] = t;
                }
            }
        }
        for (int i = 0; i < outCount; i++) out[i] = temp[i];
    }

    // 获取个人历史成绩
    int getUserHistory(const char* username, ScoreRecord* out, int maxCount) {
        int count = 0;
        for (int i = scoreCount - 1; i >= 0 && count < maxCount; i--) {
            if (strcmp(scores[i].username, username) == 0) {
                out[count++] = scores[i];
            }
        }
        return count;
    }

    int getPersonalBest(const char* username) {
        int best = 0;
        for (int i = 0; i < scoreCount; i++)
            if (strcmp(scores[i].username, username) == 0)
                if (scores[i].wpm > best) best = scores[i].wpm;
        return best;
    }

    int getScoreCount() { return scoreCount; }
};

/* ============================================================
 *  类6: GameSettings —— 游戏设置类
 * ============================================================ */
class GameSettings {
public:
    int  practiceTime;
    int  difficulty;
    int  targetSpeed;
    bool showKeyboard;
    bool soundEnabled;
    int  effectType;       // 特效类型: 0=无, 1=闪烁, 2=粒子

    GameSettings() {
        practiceTime = 60;
        difficulty = DIFF_NORMAL;
        targetSpeed = 40;
        showKeyboard = true;
        soundEnabled = true;
        effectType = EFT_FLASH;
    }

    void loadSettings() {
        FILE* fp = fopen(CONFIG_FILE, "rb");
        if (fp != NULL) {
            fread(this, sizeof(GameSettings), 1, fp);
            fclose(fp);
        }
    }

    void saveSettings() {
        FILE* fp = fopen(CONFIG_FILE, "wb");
        if (fp != NULL) {
            fwrite(this, sizeof(GameSettings), 1, fp);
            fclose(fp);
        }
    }

    const char* getDifficultyText() {
        switch (difficulty) {
            case DIFF_EASY:   return "简单";
            case DIFF_NORMAL: return "普通";
            case DIFF_HARD:   return "困难";
            default:          return "普通";
        }
    }

    const char* getEffectText() {
        switch (effectType) {
            case EFT_NONE:     return "关闭";
            case EFT_FLASH:    return "闪烁";
            case EFT_PARTICLE: return "粒子";
            default:           return "闪烁";
        }
    }

    int getWordCount() {
        switch (difficulty) {
            case DIFF_EASY:   return 30;
            case DIFF_NORMAL: return 50;
            case DIFF_HARD:   return 80;
            default:          return 50;
        }
    }
};

/* ============================================================
 *  类7: KeyboardTrainer —— 键盘练习核心逻辑类
 * ============================================================ */
class KeyboardTrainer {
private:
    char targetText[256];
    char userInput[256];
    int  inputLen;
    int  totalChars;
    int  correctChars;
    int  startTime;
    int  elapsedTime;
    bool isRunning;
    int  lastKeyTime;     // 上次按键时间（用于计算响应时间）
    int  currentKeyStart; // 当前按键开始时间

    const char* easyWords[30] = {
        "apple", "book", "cat", "dog", "egg", "fish", "game", "hat",
        "ice", "jump", "kite", "lion", "moon", "nest", "orange", "pen",
        "queen", "red", "sun", "tree", "up", "van", "water", "xray",
        "yellow", "zoo", "happy", "good", "nice", "big"
    };

    const char* normalWords[50] = {
        "computer", "program", "keyboard", "screen", "mouse", "printer",
        "software", "hardware", "internet", "network", "server", "client",
        "database", "memory", "processor", "graphic", "display", "window",
        "system", "application", "function", "variable", "constant", "string",
        "number", "boolean", "object", "array", "class", "method",
        "public", "private", "static", "return", "import", "package",
        "module", "library", "framework", "project", "version", "control",
        "source", "target", "build", "compile", "debug", "test", "release"
    };

    const char* hardWords[40] = {
        "algorithm", "encryption", "optimization", "synchronization",
        "parallelism", "concurrency", "asynchronous", "multithreading",
        "virtualization", "containerization", "microservices", "scalability",
        "architecture", "implementation", "development", "deployment",
        "configuration", "authentication", "authorization", "dependency",
        "inheritance", "polymorphism", "encapsulation", "abstraction",
        "recursion", "iteration", "traversal", "manipulation", "transformation",
        "visualization", "notification", "subscription", "distribution",
        "integration", "coordination", "evaluation", "verification"
    };

public:
    KeyboardTrainer() {
        reset();
    }

    void reset() {
        targetText[0] = '\0';
        userInput[0] = '\0';
        inputLen = 0;
        totalChars = 0;
        correctChars = 0;
        startTime = 0;
        elapsedTime = 0;
        isRunning = false;
        lastKeyTime = 0;
        currentKeyStart = 0;
    }

    void generateText(int difficulty, int wordCount) {
        targetText[0] = '\0';
        srand((unsigned)time(NULL));
        for (int i = 0; i < wordCount; i++) {
            const char* word = NULL;
            if (difficulty == DIFF_EASY) {
                word = easyWords[rand() % 30];
            } else if (difficulty == DIFF_HARD) {
                word = hardWords[rand() % 37];
            } else {
                word = normalWords[rand() % 50];
            }
            if (i > 0) strcat(targetText, " ");
            strcat(targetText, word);
        }
        userInput[0] = '\0';
        inputLen = 0;
        totalChars = 0;
        correctChars = 0;
        isRunning = true;
        startTime = (int)clock();
        lastKeyTime = startTime;
        currentKeyStart = startTime;
    }

    // 处理输入，返回是否正确，用于特效触发
    bool processInput(char ch, KeyStatsManager* keyStats) {
        if (!isRunning) return false;
        if (ch == '\b') {
            if (inputLen > 0) {
                inputLen--;
                userInput[inputLen] = '\0';
            }
            return false;
        }
        if (ch == '\r' || ch == '\n') return false;
        if (inputLen < 255 && inputLen < (int)strlen(targetText)) {
            int now = (int)clock();
            int responseTime = (now - lastKeyTime) * 1000 / CLOCKS_PER_SEC;
            if (responseTime < 0) responseTime = 0;
            if (responseTime > 5000) responseTime = 5000;
            
            userInput[inputLen] = ch;
            inputLen++;
            userInput[inputLen] = '\0';
            totalChars++;
            
            bool correct = (ch == targetText[inputLen - 1]);
            if (correct) correctChars++;
            
            // 记录按键习惯
            if (keyStats != NULL) {
                keyStats->recordPress(ch, correct, responseTime);
            }
            
            lastKeyTime = now;
            return correct;
        }
        return false;
    }

    void updateTime() {
        if (isRunning) {
            elapsedTime = (int)(clock() - startTime) / CLOCKS_PER_SEC;
        }
    }

    int getWPM() {
        if (elapsedTime <= 0) return 0;
        return (int)((correctChars / 5.0) * 60.0 / elapsedTime);
    }

    int getAccuracy() {
        if (totalChars <= 0) return 0;
        return (int)(correctChars * 100.0 / totalChars);
    }

    bool isFinished() {
        return inputLen >= (int)strlen(targetText);
    }

    void stop() { isRunning = false; }

    const char* getTargetText() { return targetText; }
    const char* getUserInput() { return userInput; }
    int getElapsedTime() { return elapsedTime; }
    bool getIsRunning() { return isRunning; }
    int getInputLen() { return inputLen; }
    int getTotalChars() { return totalChars; }
    int getCorrectChars() { return correctChars; }
};

/* ============================================================
 *  特效粒子结构
 * ============================================================ */
struct Particle {
    float x, y;
    float vx, vy;
    int life;
    int maxLife;
    COLORREF color;
    bool active;
};

#define MAX_PARTICLES 100

/* ============================================================
 *  类8: UIManager —— 界面管理类（基于 easyx 图形库）
 * ============================================================ */
class UIManager {
private:
    int mouseX, mouseY;
    Particle particles[MAX_PARTICLES];
    int flashTimer;       // 闪烁计时器
    bool flashCorrect;    // 闪烁类型（正确/错误）
    int flashX, flashY;   // 闪烁位置

public:
    UIManager() {
        mouseX = mouseY = 0;
        flashTimer = 0;
        flashCorrect = true;
        flashX = flashY = 0;
        for (int i = 0; i < MAX_PARTICLES; i++)
            particles[i].active = false;
    }

    void initGraphics() {
        initgraph(SCREEN_W, SCREEN_H, SHOWCONSOLE);
        setbkcolor(RGB(240, 248, 255));
        cleardevice();
        setlinestyle(PS_SOLID, 2);
    }

    void closeGraphics() {
        closegraph();
    }

    void updateMouse() {
        mouseX = mousex();
        mouseY = mousey();
    }

    bool isMouseInRect(int x, int y, int w, int h) {
        return (mouseX >= x && mouseX <= x + w &&
                mouseY >= y && mouseY <= y + h);
    }

    void clearScreen() {
        setbkcolor(RGB(240, 248, 255));
        cleardevice();
    }

    // ========== 基础绘制 ==========
    void drawButton(int x, int y, int w, int h, const char* text,
                    COLORREF bgColor, COLORREF textColor, int fontSize) {
        setfillcolor(bgColor);
        fillroundrect(x, y, x + w, y + h, 10, 10);
        setbkmode(TRANSPARENT);
        settextcolor(textColor);
        settextstyle(fontSize, 0, _T("微软雅黑"));
        RECT r = { x, y, x + w, y + h };
        drawtext(text, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    void drawInputBox(int x, int y, int w, int h, const char* label,
                      const char* value, bool isPassword) {
        settextcolor(RGB(50, 50, 50));
        settextstyle(18, 0, _T("微软雅黑"));
        outtextxy(x, y - 22, label);
        setfillcolor(RGB(255, 255, 255));
        fillroundrect(x, y, x + w, y + h, 6, 6);
        setlinecolor(RGB(180, 180, 180));
        roundrect(x, y, x + w, y + h, 6, 6);
        settextcolor(RGB(30, 30, 30));
        settextstyle(18, 0, _T("Consolas"));
        if (isPassword && value[0] != '\0') {
            char mask[64] = {0};
            int len = strlen(value);
            if (len > 30) len = 30;
            for (int i = 0; i < len; i++) mask[i] = '*';
            mask[len] = '\0';
            outtextxy(x + 10, y + 8, mask);
        } else {
            outtextxy(x + 10, y + 8, value);
        }
    }

    void drawTitle(const char* text, int y) {
        settextcolor(RGB(30, 60, 100));
        settextstyle(36, 0, _T("微软雅黑"));
        RECT r = { 0, y, SCREEN_W, y + 50 };
        drawtext(text, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    void drawMessage(const char* msg, int y, COLORREF color) {
        settextcolor(color);
        settextstyle(16, 0, _T("微软雅黑"));
        RECT r = { 0, y, SCREEN_W, y + 30 };
        drawtext(msg, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    void drawCard(int x, int y, int w, int h, const char* title,
                  const char* content) {
        setfillcolor(RGB(255, 255, 255));
        fillroundrect(x, y, x + w, y + h, 12, 12);
        setlinecolor(RGB(200, 210, 220));
        roundrect(x, y, x + w, y + h, 12, 12);
        settextcolor(RGB(30, 60, 100));
        settextstyle(20, 0, _T("微软雅黑"));
        outtextxy(x + 15, y + 12, title);
        settextcolor(RGB(80, 80, 80));
        settextstyle(16, 0, _T("微软雅黑"));
        RECT r = { x + 15, y + 40, x + w - 15, y + h - 10 };
        drawtext(content, &r, DT_LEFT | DT_TOP | DT_WORDBREAK);
    }

    void drawProgressBar(int x, int y, int w, int h, int percent) {
        setfillcolor(RGB(220, 220, 220));
        fillroundrect(x, y, x + w, y + h, h / 2, h / 2);
        int fillW = (int)(w * percent / 100.0);
        if (fillW > w) fillW = w;
        setfillcolor(RGB(70, 160, 240));
        fillroundrect(x, y, x + fillW, y + h, h / 2, h / 2);
    }

    // ========== 特效系统 ==========
    void triggerEffect(int x, int y, bool correct, int effectType) {
        if (effectType == EFT_NONE) return;
        if (effectType == EFT_FLASH) {
            flashTimer = 10;
            flashCorrect = correct;
            flashX = x;
            flashY = y;
        } else if (effectType == EFT_PARTICLE) {
            spawnParticles(x, y, correct);
        }
    }

    void spawnParticles(int x, int y, bool correct) {
        COLORREF color = correct ? RGB(50, 220, 50) : RGB(220, 50, 50);
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < MAX_PARTICLES; j++) {
                if (!particles[j].active) {
                    particles[j].x = x + 6;
                    particles[j].y = y + 10;
                    float angle = (rand() % 360) * 3.14159f / 180.0f;
                    float speed = 1.0f + (rand() % 30) / 10.0f;
                    particles[j].vx = cos(angle) * speed;
                    particles[j].vy = sin(angle) * speed;
                    particles[j].life = 20 + rand() % 15;
                    particles[j].maxLife = particles[j].life;
                    particles[j].color = color;
                    particles[j].active = true;
                    break;
                }
            }
        }
    }

    void updateAndDrawParticles() {
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].active) {
                particles[i].x += particles[i].vx;
                particles[i].y += particles[i].vy;
                particles[i].vy += 0.3f;  // 重力
                particles[i].life--;
                if (particles[i].life <= 0) {
                    particles[i].active = false;
                    continue;
                }
                int alpha = (int)(255 * particles[i].life / particles[i].maxLife);
                int r = (int)((particles[i].color & 0xFF) * alpha / 255);
                int g = (int)(((particles[i].color >> 8) & 0xFF) * alpha / 255);
                int b = (int)(((particles[i].color >> 16) & 0xFF) * alpha / 255);
                setfillcolor(RGB(r, g, b));
                solidcircle((int)particles[i].x, (int)particles[i].y, 2 + particles[i].life / 8);
            }
        }
    }

    void drawFlashEffect() {
        if (flashTimer > 0) {
            flashTimer--;
            COLORREF color = flashCorrect ? RGB(50, 220, 50) : RGB(220, 50, 50);
            int alpha = flashTimer * 25;
            setfillcolor(color);
            setlinecolor(color);
            int r = (11 - flashTimer) * 3;
            circle(flashX + 6, flashY + 10, r);
        }
    }

    void updateEffects() {
        updateAndDrawParticles();
        drawFlashEffect();
    }

    // ========== 打字文本绘制（带特效触发） ==========
    void drawTypingText(const char* target, const char* input, int y,
                        int effectType, int* effectX, int* effectY, bool* effectCorrect, bool* effectTriggered) {
        settextcolor(RGB(50, 50, 50));
        settextstyle(20, 0, _T("Consolas"));
        int x = 60;
        int len = strlen(target);
        for (int i = 0; i < len; i++) {
            if (i < strlen(input)) {
                if (input[i] == target[i])
                    settextcolor(RGB(50, 180, 50));
                else
                    settextcolor(RGB(220, 50, 50));
            } else if (i == strlen(input)) {
                settextcolor(RGB(70, 130, 220));
            } else {
                settextcolor(RGB(120, 120, 120));
            }
            char buf[4] = { target[i], '\0' };
            int drawX = x;
            int drawY = y;
            outtextxy(drawX, drawY, buf);
            
            // 记录特效位置
            if (i == strlen(input) - 1 && effectTriggered != NULL && *effectTriggered) {
                *effectX = drawX;
                *effectY = drawY;
                *effectTriggered = false;
            }
            
            x += 13;
            if (x > SCREEN_W - 60) { x = 60; y += 28; }
        }
    }

    void drawKeyboardHint(int x, int y, char targetKey) {
        setfillcolor(RGB(230, 240, 255));
        fillroundrect(x, y, x + 40, y + 40, 6, 6);
        setlinecolor(RGB(100, 150, 220));
        roundrect(x, y, x + 40, y + 40, 6, 6);
        settextcolor(RGB(30, 60, 120));
        settextstyle(18, 0, _T("Consolas"));
        char buf[4] = { targetKey, '\0' };
        RECT r = { x, y, x + 40, y + 40 };
        drawtext(buf, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // ========== 历史成绩表格绘制 ==========
    void drawHistoryTable(ScoreRecord* records, int count, int startY) {
        settextcolor(RGB(50, 50, 50));
        settextstyle(16, 0, _T("微软雅黑"));
        outtextxy(60, startY, _T("日期"));
        outtextxy(200, startY, _T("WPM"));
        outtextxy(280, startY, _T("准确率"));
        outtextxy(380, startY, _T("时长"));
        outtextxy(480, startY, _T("难度"));
        outtextxy(580, startY, _T("字数"));
        
        setlinecolor(RGB(200, 200, 200));
        line(50, startY + 25, 700, startY + 25);
        
        for (int i = 0; i < count; i++) {
            int y = startY + 35 + i * 32;
            char buf[64];
            
            // 日期格式化
            time_t t = records[i].scoreDate;
            struct tm* tmInfo = localtime(&t);
            if (tmInfo != NULL) {
                sprintf(buf, "%02d/%02d %02d:%02d", 
                        tmInfo->tm_mon + 1, tmInfo->tm_mday,
                        tmInfo->tm_hour, tmInfo->tm_min);
            } else {
                strcpy(buf, "--");
            }
            outtextxy(60, y, buf);
            
            sprintf(buf, "%d", records[i].wpm);
            outtextxy(200, y, buf);
            sprintf(buf, "%d%%", records[i].accuracy);
            outtextxy(280, y, buf);
            sprintf(buf, "%ds", records[i].duration);
            outtextxy(380, y, buf);
            outtextxy(480, y, records[i].difficulty == DIFF_EASY ? "简单" :
                             records[i].difficulty == DIFF_HARD ? "困难" : "普通");
            sprintf(buf, "%d/%d", records[i].correctChars, records[i].totalChars);
            outtextxy(580, y, buf);
        }
    }

    // ========== 按键习惯绘制 ==========
    void drawKeyStats(KeyStatsManager* keyStats, int startY) {
        settextcolor(RGB(50, 50, 50));
        settextstyle(18, 0, _T("微软雅黑"));
        outtextxy(60, startY, _T("个人按键习惯分析"));
        
        char buf[128];
        char best = keyStats->getBestKey();
        char worst = keyStats->getWeakestKey();
        int bestAcc = keyStats->getAccuracy(best);
        int worstAcc = keyStats->getAccuracy(worst);
        
        settextstyle(16, 0, _T("微软雅黑"));
        sprintf(buf, "最熟练按键: '%c' (准确率 %d%%)", best, bestAcc);
        outtextxy(60, startY + 35, buf);
        sprintf(buf, "需加强按键: '%c' (准确率 %d%%)", worst, worstAcc);
        outtextxy(60, startY + 65, buf);
        
        // 绘制常用按键热图
        settextstyle(14, 0, _T("Consolas"));
        outtextxy(60, startY + 100, _T("常用按键分布 (按准确率着色):"));
        
        char keys[] = "abcdefghijklmnopqrstuvwxyz";
        int x = 60, y = startY + 130;
        for (int i = 0; i < 26; i++) {
            int acc = keyStats->getAccuracy(keys[i]);
            if (acc >= 90) settextcolor(RGB(50, 180, 50));
            else if (acc >= 70) settextcolor(RGB(240, 170, 60));
            else if (acc > 0) settextcolor(RGB(220, 50, 50));
            else settextcolor(RGB(180, 180, 180));
            
            char kb[4] = { keys[i], '\0' };
            outtextxy(x, y, kb);
            x += 28;
            if (x > 700) { x = 60; y += 25; }
        }
    }

    // ========== 结果页面绘制 ==========
    void drawResultPage(int wpm, int accuracy, int duration, int totalChars, int correctChars,
                        KeyStatsManager* keyStats, int bestWPM) {
        clearScreen();
        drawTitle("练习结果", 40);
        
        // 大分数展示
        char buf[64];
        settextcolor(RGB(30, 60, 100));
        settextstyle(48, 0, _T("Inter, MiSans"));
        sprintf(buf, "%d", wpm);
        outtextxy(200, 120, buf);
        settextstyle(20, 0, _T("微软雅黑"));
        outtextxy(200, 180, _T("WPM"));
        
        settextstyle(48, 0, _T("Inter, MiSans"));
        sprintf(buf, "%d%%", accuracy);
        outtextxy(480, 120, buf);
        settextstyle(20, 0, _T("微软雅黑"));
        outtextxy(480, 180, _T("准确率"));
        
        // 详细数据卡片
        settextstyle(16, 0, _T("微软雅黑"));
        settextcolor(RGB(80, 80, 80));
        sprintf(buf, "练习时长: %d秒  |  正确字数: %d/%d  |  个人最佳: %d WPM", 
                duration, correctChars, totalChars, bestWPM);
        outtextxy(60, 220, buf);
        
        // 按键习惯分析
        if (keyStats != NULL) {
            drawKeyStats(keyStats, 260);
        }
        
        // 按钮
        drawButton(300, 520, 200, 48, "返回主菜单",
                   RGB(70, 160, 240), RGB(255, 255, 255), 20);
    }
};

/* ============================================================
 *  主函数 —— 程序入口与页面路由
 * ============================================================ */
int main() {
    UserManager       userMgr;
    ScoreManager      scoreMgr;
    GameSettings      settings;
    KeyboardTrainer   trainer;
    UIManager         ui;
    KeyStatsManager   keyStats;

    settings.loadSettings();
    ui.initGraphics();

    PageType currentPage = PAGE_LOGIN;
    char inputBuf1[64] = {0}, inputBuf2[64] = {0}, inputBuf3[64] = {0};
    int  inputFocus = 0;
    char msgBuf[128] = {0};
    int  msgTimer = 0;
    
    // 练习结果缓存
    int lastWPM = 0, lastAccuracy = 0, lastDuration = 0;
    int lastTotalChars = 0, lastCorrectChars = 0;
    
    // 特效触发状态
    int effectX = 0, effectY = 0;
    bool effectCorrect = false;
    bool effectTriggered = false;

    while (currentPage != PAGE_EXIT) {
        ui.updateMouse();
        
        // 练习页面不清屏（保留特效轨迹），其他页面清屏
        if (currentPage != PAGE_PRACTICE) {
            ui.clearScreen();
        } else {
            // 练习页面局部刷新背景
            setbkcolor(RGB(240, 248, 255));
            cleardevice();
        }
        
        // 更新特效
        if (currentPage == PAGE_PRACTICE) {
            ui.updateEffects();
        }

        // ========== 登录页面 ==========
        if (currentPage == PAGE_LOGIN) {
            ui.drawTitle("键盘练习系统 - 用户登录", 60);
            ui.drawInputBox(280, 160, 240, 36, "用户名:", inputBuf1, false);
            ui.drawInputBox(280, 240, 240, 36, "密码:",   inputBuf2, true);
            ui.drawButton(280, 320, 110, 42, "登录",
                          RGB(70, 160, 240), RGB(255, 255, 255), 18);
            ui.drawButton(410, 320, 110, 42, "注册",
                          RGB(100, 180, 100), RGB(255, 255, 255), 18);
            if (msgTimer > 0) {
                ui.drawMessage(msgBuf, 400, RGB(220, 60, 60));
                msgTimer--;
            }

            if (MouseHit()) {
                MOUSEMSG m = GetMouseMsg();
                if (m.uMsg == WM_LBUTTONDOWN) {
                    if (ui.isMouseInRect(280, 160, 240, 36)) inputFocus = 1;
                    else if (ui.isMouseInRect(280, 240, 240, 36)) inputFocus = 2;
                    else if (ui.isMouseInRect(280, 320, 110, 42)) {
                        if (strlen(inputBuf1) < 1 || strlen(inputBuf2) < 1) {
                            strcpy(msgBuf, "用户名和密码不能为空!");
                            msgTimer = 60;
                        } else {
                            int idx = userMgr.login(inputBuf1, inputBuf2);
                            if (idx >= 0) {
                                strcpy(msgBuf, "登录成功!");
                                keyStats.setOwner(userMgr.getCurrentUsername());
                                inputBuf1[0] = inputBuf2[0] = '\0';
                                currentPage = PAGE_MAIN_MENU;
                            } else {
                                strcpy(msgBuf, "用户名或密码错误!");
                                msgTimer = 60;
                            }
                        }
                    } else if (ui.isMouseInRect(410, 320, 110, 42)) {
                        inputBuf1[0] = inputBuf2[0] = '\0';
                        currentPage = PAGE_REGISTER;
                    }
                }
            }
        }
        // ========== 注册页面 ==========
        else if (currentPage == PAGE_REGISTER) {
            ui.drawTitle("新用户注册", 60);
            ui.drawInputBox(280, 140, 240, 36, "用户名:", inputBuf1, false);
            ui.drawInputBox(280, 220, 240, 36, "密码:",   inputBuf2, true);
            ui.drawInputBox(280, 300, 240, 36, "昵称:",   inputBuf3, false);
            ui.drawButton(280, 380, 110, 42, "确认注册",
                          RGB(70, 160, 240), RGB(255, 255, 255), 18);
            ui.drawButton(410, 380, 110, 42, "返回",
                          RGB(150, 150, 150), RGB(255, 255, 255), 18);
            if (msgTimer > 0) {
                ui.drawMessage(msgBuf, 460, RGB(220, 60, 60));
                msgTimer--;
            }

            if (MouseHit()) {
                MOUSEMSG m = GetMouseMsg();
                if (m.uMsg == WM_LBUTTONDOWN) {
                    if (ui.isMouseInRect(280, 140, 240, 36)) inputFocus = 1;
                    else if (ui.isMouseInRect(280, 220, 240, 36)) inputFocus = 2;
                    else if (ui.isMouseInRect(280, 300, 240, 36)) inputFocus = 3;
                    else if (ui.isMouseInRect(280, 380, 110, 42)) {
                        int ret = userMgr.registerUser(inputBuf1, inputBuf2, inputBuf3);
                        if (ret == 0) {
                            strcpy(msgBuf, "注册成功!");
                            inputBuf1[0] = inputBuf2[0] = inputBuf3[0] = '\0';
                            currentPage = PAGE_LOGIN;
                        } else if (ret == -2) {
                            strcpy(msgBuf, "用户名已存在!");
                            msgTimer = 60;
                        } else if (ret == -3) {
                            strcpy(msgBuf, "用户名/密码至少2位!");
                            msgTimer = 60;
                        } else {
                            strcpy(msgBuf, "用户数量已达上限!");
                            msgTimer = 60;
                        }
                    } else if (ui.isMouseInRect(410, 380, 110, 42)) {
                        inputBuf1[0] = inputBuf2[0] = inputBuf3[0] = '\0';
                        currentPage = PAGE_LOGIN;
                    }
                }
            }
        }
        // ========== 主菜单 ==========
        else if (currentPage == PAGE_MAIN_MENU) {
            ui.drawTitle("键盘练习 - 主菜单", 50);
            char welcome[128];
            sprintf(welcome, "欢迎, %s", userMgr.getCurrentUsername());
            settextcolor(RGB(80, 80, 80));
            settextstyle(18, 0, _T("微软雅黑"));
            outtextxy(30, 30, welcome);

            ui.drawButton(300, 130, 200, 44, "开始练习",
                          RGB(70, 160, 240), RGB(255, 255, 255), 20);
            ui.drawButton(300, 190, 200, 44, "成绩排名",
                          RGB(100, 180, 100), RGB(255, 255, 255), 20);
            ui.drawButton(300, 250, 200, 44, "历史成绩",
                          RGB(150, 120, 200), RGB(255, 255, 255), 20);
            ui.drawButton(300, 310, 200, 44, "练习设置",
                          RGB(240, 170, 60), RGB(255, 255, 255), 20);
            ui.drawButton(300, 370, 200, 44, "个人资料",
                          RGB(120, 180, 200), RGB(255, 255, 255), 20);
            ui.drawButton(300, 430, 200, 44, "退出登录",
                          RGB(180, 80, 80), RGB(255, 255, 255), 20);

            if (MouseHit()) {
                MOUSEMSG m = GetMouseMsg();
                if (m.uMsg == WM_LBUTTONDOWN) {
                    if (ui.isMouseInRect(300, 130, 200, 44)) {
                        trainer.reset();
                        trainer.generateText(settings.difficulty, settings.getWordCount());
                        effectTriggered = false;
                        currentPage = PAGE_PRACTICE;
                    } else if (ui.isMouseInRect(300, 190, 200, 44))
                        currentPage = PAGE_RANKING;
                    else if (ui.isMouseInRect(300, 250, 200, 44))
                        currentPage = PAGE_HISTORY;
                    else if (ui.isMouseInRect(300, 310, 200, 44))
                        currentPage = PAGE_SETTINGS;
                    else if (ui.isMouseInRect(300, 370, 200, 44))
                        currentPage = PAGE_PROFILE;
                    else if (ui.isMouseInRect(300, 430, 200, 44)) {
                        userMgr.logout();
                        currentPage = PAGE_LOGIN;
                    }
                }
            }
        }
        // ========== 练习页面 ==========
        else if (currentPage == PAGE_PRACTICE) {
            trainer.updateTime();
            ui.drawTitle("键盘练习中...", 30);

            char info[128];
            sprintf(info, "时间: %d秒 / 目标: %d秒  |  WPM: %d  |  准确率: %d%%",
                    trainer.getElapsedTime(), settings.practiceTime,
                    trainer.getWPM(), trainer.getAccuracy());
            settextcolor(RGB(60, 60, 60));
            settextstyle(16, 0, _T("微软雅黑"));
            outtextxy(60, 80, info);

            int progress = (int)(trainer.getElapsedTime() * 100.0 / settings.practiceTime);
            if (progress > 100) progress = 100;
            ui.drawProgressBar(60, 110, 680, 16, progress);

            ui.drawTypingText(trainer.getTargetText(), trainer.getUserInput(), 160,
                              settings.effectType, &effectX, &effectY, &effectCorrect, &effectTriggered);
            
            // 触发特效
            if (effectX > 0 && effectY > 0) {
                ui.triggerEffect(effectX, effectY, effectCorrect, settings.effectType);
                effectX = effectY = 0;
            }

            if (settings.showKeyboard && trainer.getInputLen() < (int)strlen(trainer.getTargetText())) {
                char nextKey = trainer.getTargetText()[trainer.getInputLen()];
                ui.drawKeyboardHint(380, 480, nextKey);
                settextcolor(RGB(120, 120, 120));
                settextstyle(14, 0, _T("微软雅黑"));
                outtextxy(340, 530, _T("下一个按键提示"));
            }

            ui.drawButton(320, 540, 160, 40, "结束练习",
                          RGB(180, 80, 80), RGB(255, 255, 255), 16);

            // 练习结束判断
            if (trainer.getElapsedTime() >= settings.practiceTime || trainer.isFinished()) {
                trainer.stop();
                lastWPM = trainer.getWPM();
                lastAccuracy = trainer.getAccuracy();
                lastDuration = trainer.getElapsedTime();
                lastTotalChars = trainer.getTotalChars();
                lastCorrectChars = trainer.getCorrectChars();
                
                scoreMgr.addScore(userMgr.getCurrentUsername(), lastWPM, lastAccuracy,
                                   lastDuration, settings.difficulty, lastTotalChars, lastCorrectChars);
                int uIdx = userMgr.getCurrentUserIndex();
                if (uIdx >= 0) {
                    User* u = userMgr.getUserByIndex(uIdx);
                    u->totalPractice++;
                    if (lastWPM > u->bestSpeed) u->bestSpeed = lastWPM;
                    userMgr.saveUsers();
                }
                keyStats.saveStats();
                currentPage = PAGE_RESULT;
            }

            if (MouseHit()) {
                MOUSEMSG m = GetMouseMsg();
                if (m.uMsg == WM_LBUTTONDOWN &&
                    ui.isMouseInRect(320, 540, 160, 40)) {
                    trainer.stop();
                    currentPage = PAGE_MAIN_MENU;
                }
            }

            if (kbhit()) {
                char ch = getch();
                if (ch == 27) {  // ESC 退出
                    trainer.stop();
                    currentPage = PAGE_MAIN_MENU;
                } else if (ch == 0 || ch == -32) {  // 功能键
                    getch();
                } else {
                    bool correct = trainer.processInput(ch, &keyStats);
                    effectCorrect = correct;
                    effectTriggered = true;
                }
            }
        }
        // ========== 结果页面 ==========
        else if (currentPage == PAGE_RESULT) {
            int bestWPM = scoreMgr.getPersonalBest(userMgr.getCurrentUsername());
            ui.drawResultPage(lastWPM, lastAccuracy, lastDuration, 
                              lastTotalChars, lastCorrectChars, &keyStats, bestWPM);
            
            if (MouseHit()) {
                MOUSEMSG m = GetMouseMsg();
                if (m.uMsg == WM_LBUTTONDOWN &&
                    ui.isMouseInRect(300, 520, 200, 48)) {
                    currentPage = PAGE_MAIN_MENU;
                }
            }
            
            if (kbhit()) {
                char ch = getch();
                if (ch == 27 || ch == '\r') currentPage = PAGE_MAIN_MENU;
            }
        }
        // ========== 排名页面 ==========
        else if (currentPage == PAGE_RANKING) {
            ui.drawTitle("成绩排行榜 (Top 10)", 50);
            ScoreRecord top[10];
            int topCount = 0;
            scoreMgr.getRanking(top, topCount, 10);

            settextcolor(RGB(50, 50, 50));
            settextstyle(18, 0, _T("微软雅黑"));
            outtextxy(120, 110, _T("排名"));
            outtextxy(220, 110, _T("用户名"));
            outtextxy(380, 110, _T("WPM"));
            outtextxy(480, 110, _T("准确率"));
            outtextxy(580, 110, _T("难度"));

            setlinecolor(RGB(200, 200, 200));
            line(100, 140, 700, 140);

            for (int i = 0; i < topCount; i++) {
                int y = 155 + i * 36;
                char buf[64];
                sprintf(buf, "%d", i + 1);
                outtextxy(120, y, buf);
                outtextxy(220, y, top[i].username);
                sprintf(buf, "%d", top[i].wpm);
                outtextxy(380, y, buf);
                sprintf(buf, "%d%%", top[i].accuracy);
                outtextxy(480, y, buf);
                outtextxy(580, y, top[i].difficulty == DIFF_EASY ? "简单" :
                                 top[i].difficulty == DIFF_HARD ? "困难" : "普通");
            }

            ui.drawButton(320, 520, 160, 40, "返回主菜单",
                          RGB(150, 150, 150), RGB(255, 255, 255), 16);
            if (MouseHit()) {
                MOUSEMSG m = GetMouseMsg();
                if (m.uMsg == WM_LBUTTONDOWN &&
                    ui.isMouseInRect(320, 520, 160, 40))
                    currentPage = PAGE_MAIN_MENU;
            }
        }
        // ========== 历史成绩页面 ==========
        else if (currentPage == PAGE_HISTORY) {
            ui.drawTitle("个人历史成绩", 50);
            
            ScoreRecord history[MAX_HISTORY];
            int hCount = scoreMgr.getUserHistory(userMgr.getCurrentUsername(), history, MAX_HISTORY);
            
            if (hCount > 0) {
                ui.drawHistoryTable(history, hCount, 100);
            } else {
                ui.drawMessage("暂无历史成绩，快去练习吧!", 250, RGB(150, 150, 150));
            }
            
            ui.drawButton(320, 520, 160, 40, "返回主菜单",
                          RGB(150, 150, 150), RGB(255, 255, 255), 16);
            if (MouseHit()) {
                MOUSEMSG m = GetMouseMsg();
                if (m.uMsg == WM_LBUTTONDOWN &&
                    ui.isMouseInRect(320, 520, 160, 40))
                    currentPage = PAGE_MAIN_MENU;
            }
        }
        // ========== 设置页面 ==========
        else if (currentPage == PAGE_SETTINGS) {
            ui.drawTitle("练习设置", 50);
            char buf[64];

            ui.drawCard(120, 120, 280, 80, "练习时长",
                        buf); sprintf(buf, "当前: %d 秒", settings.practiceTime);
            ui.drawCard(120, 120, 280, 80, "练习时长", buf);
            ui.drawButton(430, 135, 50, 36, "+",
                          RGB(70, 160, 240), RGB(255, 255, 255), 18);
            ui.drawButton(490, 135, 50, 36, "-",
                          RGB(240, 100, 100), RGB(255, 255, 255), 18);

            ui.drawCard(120, 220, 280, 80, "难度级别",
                        buf); sprintf(buf, "当前: %s", settings.getDifficultyText());
            ui.drawCard(120, 220, 280, 80, "难度级别", buf);
            ui.drawButton(430, 235, 120, 36, "切换难度",
                          RGB(70, 160, 240), RGB(255, 255, 255), 16);

            ui.drawCard(120, 320, 280, 80, "键盘提示",
                        settings.showKeyboard ? "当前: 开启" : "当前: 关闭");
            ui.drawButton(430, 335, 120, 36, "开关",
                          RGB(70, 160, 240), RGB(255, 255, 255), 16);
            
            ui.drawCard(120, 420, 280, 80, "打字特效",
                        buf); sprintf(buf, "当前: %s", settings.getEffectText());
            ui.drawCard(120, 420, 280, 80, "打字特效", buf);
            ui.drawButton(430, 435, 120, 36, "切换特效",
                          RGB(70, 160, 240), RGB(255, 255, 255), 16);

            ui.drawButton(300, 530, 200, 44, "保存并返回",
                          RGB(100, 180, 100), RGB(255, 255, 255), 18);

            if (MouseHit()) {
                MOUSEMSG m = GetMouseMsg();
                if (m.uMsg == WM_LBUTTONDOWN) {
                    if (ui.isMouseInRect(430, 135, 50, 36) && settings.practiceTime < 300)
                        settings.practiceTime += 10;
                    else if (ui.isMouseInRect(490, 135, 50, 36) && settings.practiceTime > 10)
                        settings.practiceTime -= 10;
                    else if (ui.isMouseInRect(430, 235, 120, 36))
                        settings.difficulty = (settings.difficulty + 1) % 3;
                    else if (ui.isMouseInRect(430, 335, 120, 36))
                        settings.showKeyboard = !settings.showKeyboard;
                    else if (ui.isMouseInRect(430, 435, 120, 36))
                        settings.effectType = (settings.effectType + 1) % 3;
                    else if (ui.isMouseInRect(300, 530, 200, 44)) {
                        settings.saveSettings();
                        currentPage = PAGE_MAIN_MENU;
                    }
                }
            }
        }
        // ========== 个人资料页面 ==========
        else if (currentPage == PAGE_PROFILE) {
            ui.drawTitle("个人资料管理", 50);
            int uIdx = userMgr.getCurrentUserIndex();
            User* u = (uIdx >= 0) ? userMgr.getUserByIndex(uIdx) : NULL;

            if (u != NULL) {
                char buf[128];
                sprintf(buf, "用户名: %s", u->username);
                ui.drawCard(120, 120, 560, 50, "基本信息", buf);
                sprintf(buf, "昵称: %s  |  总练习: %d次  |  最高: %d WPM  |  最佳: %d WPM",
                        u->nickname, u->totalPractice, u->bestSpeed,
                        scoreMgr.getPersonalBest(u->username));
                ui.drawCard(120, 190, 560, 70, "详细数据", buf);
                
                // 按键习惯展示
                ui.drawKeyStats(&keyStats, 280);

                ui.drawInputBox(280, 420, 240, 36, "新昵称:", inputBuf1, false);
                ui.drawInputBox(280, 500, 240, 36, "新密码:", inputBuf2, true);
                ui.drawButton(280, 560, 110, 36, "保存修改",
                              RGB(70, 160, 240), RGB(255, 255, 255), 16);
                ui.drawButton(410, 560, 110, 36, "返回",
                              RGB(150, 150, 150), RGB(255, 255, 255), 16);

                if (msgTimer > 0) {
                    ui.drawMessage(msgBuf, 500, RGB(50, 180, 50));
                    msgTimer--;
                }

                if (MouseHit()) {
                    MOUSEMSG m = GetMouseMsg();
                    if (m.uMsg == WM_LBUTTONDOWN) {
                        if (ui.isMouseInRect(280, 420, 240, 36)) inputFocus = 1;
                        else if (ui.isMouseInRect(280, 500, 240, 36)) inputFocus = 2;
                        else if (ui.isMouseInRect(280, 560, 110, 36)) {
                            userMgr.updateProfile(uIdx, inputBuf1, inputBuf2);
                            strcpy(msgBuf, "资料修改成功!");
                            inputBuf1[0] = inputBuf2[0] = '\0';
                            msgTimer = 60;
                        } else if (ui.isMouseInRect(410, 560, 110, 36)) {
                            inputBuf1[0] = inputBuf2[0] = '\0';
                            currentPage = PAGE_MAIN_MENU;
                        }
                    }
                }
            }
        }

        // 通用键盘输入处理（非练习/结果页面）
        if (currentPage != PAGE_PRACTICE && currentPage != PAGE_RESULT && kbhit()) {
            char ch = getch();
            if (ch == '\r' || ch == '\n') {
                // 回车忽略
            } else if (ch == '\b') {
                if (inputFocus == 1 && strlen(inputBuf1) > 0)
                    inputBuf1[strlen(inputBuf1) - 1] = '\0';
                else if (inputFocus == 2 && strlen(inputBuf2) > 0)
                    inputBuf2[strlen(inputBuf2) - 1] = '\0';
                else if (inputFocus == 3 && strlen(inputBuf3) > 0)
                    inputBuf3[strlen(inputBuf3) - 1] = '\0';
            } else if (ch >= 32 && ch <= 126) {
                if (inputFocus == 1 && strlen(inputBuf1) < 30) {
                    int len = strlen(inputBuf1); inputBuf1[len] = ch; inputBuf1[len + 1] = '\0';
                } else if (inputFocus == 2 && strlen(inputBuf2) < 30) {
                    int len = strlen(inputBuf2); inputBuf2[len] = ch; inputBuf2[len + 1] = '\0';
                } else if (inputFocus == 3 && strlen(inputBuf3) < 30) {
                    int len = strlen(inputBuf3); inputBuf3[len] = ch; inputBuf3[len + 1] = '\0';
                }
            }
        }

        Sleep(16);
        FlushMouseMsgBuffer();
    }

    ui.closeGraphics();
    return 0;
}
