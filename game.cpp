#include <SFML/Graphics.hpp>  // Thư viện đồ họa chính
#include <list>               // Sử dụng danh sách liên kết
#include <cmath>              // Các hàm toán học
#include <cstdlib>            // Hàm ngẫu nhiên
#include <ctime>              // Thời gian
#include <fstream>            // Đọc/ghi file
#include <sstream>            // Xử lý chuỗi

using namespace sf;
using namespace std;

const int W = 1200;           // Chiều rộng cửa sổ
const int H = 800;            // Chiều cao cửa sổ
float DEGTORAD = 0.017453f;   // Hệ số chuyển độ sang radian

// Thêm các biến điểm và font
int score = 0;                // Điểm hiện tại
int highScore = 0;            // Điểm cao nhất
Font font;                    // Để hiển thị text

// Quản lý spawn thiên thạch
float asteroidSpawnTimer = 0;
const float asteroidSpawnInterval = 1.0f; // Thời gian giữa các lần spawn (giây)
const int maxAsteroids = 20; // Số thiên thạch tối đa

// Trạng thái game
enum GameState {
    START_SCREEN,             // Màn hình bắt đầu
    PLAYING,                  // Đang chơi
    PAUSED,                   // Tạm dừng
    GAME_OVER                 // Kết thúc game
};

// Thêm enum chọn chế độ scale
enum ScaleMode {
    FIT,    // Giữ tỉ lệ, có thể có viền
    FILL,   // Phủ đầy, có thể méo
    CROP    // Phủ đầy và crop phần thừa
};

// Hàm scale thông minh
void smartScale(Sprite& sprite, int width, int height, ScaleMode mode = FIT) {
    FloatRect bounds = sprite.getLocalBounds();

    switch (mode) {
    case FIT: {
        float scale = std::min(width / bounds.width, height / bounds.height);
        sprite.setScale(scale, scale);
        sprite.setPosition(width / 2, height / 2);
        sprite.setOrigin(bounds.width / 2, bounds.height / 2);
        break;
    }
    case FILL: {
             sprite.setScale(width / bounds.width, height / bounds.height);
             sprite.setPosition(0, 0);
             break;
    }
    case CROP: {
        float scale = std::max(width / bounds.width, height / bounds.height);
        sprite.setScale(scale, scale);
        sprite.setPosition(width / 2, height / 2);
        sprite.setOrigin(bounds.width / 2, bounds.height / 2);
        break;
    }
    }
}

// Vẽ bảng điểm
void drawScoreboard(RenderWindow& app, Font& font, int score, int highScore, GameState state) {
    Text scoreText;
    scoreText.setFont(font);
    scoreText.setString("Score: " + to_string(score));
    scoreText.setCharacterSize(30);
    scoreText.setFillColor(Color::White);
    scoreText.setPosition(20, 20);

    Text highScoreText;
    highScoreText.setFont(font);
    highScoreText.setString("High Score: " + to_string(highScore));
    highScoreText.setCharacterSize(30);
    highScoreText.setFillColor(Color::Yellow);
    highScoreText.setPosition(20, 60);

    app.draw(scoreText);
    app.draw(highScoreText);

    if (state == GAME_OVER) {
        Text gameOverText;
        gameOverText.setFont(font);
        gameOverText.setString("GAME OVER\nPress Enter to Restart");
        gameOverText.setCharacterSize(40);
        gameOverText.setFillColor(Color::Red);
        gameOverText.setPosition(W / 2 - 150, H / 2);
        gameOverText.setStyle(Text::Bold);
        app.draw(gameOverText);
    }

    if (state == PAUSED) {
        Text pauseText;
        pauseText.setFont(font);
        pauseText.setString("PAUSED\nPress P to Continue");
        pauseText.setCharacterSize(40);
        pauseText.setFillColor(Color::Green);
        pauseText.setPosition(W / 2 - 150, H / 2 - 100);
        pauseText.setStyle(Text::Bold);
        app.draw(pauseText);
    }
}

// Hàm đọc high score từ file
void loadHighScore(int& highScore) {
    ifstream inputFile("highscore.dat");
    if (inputFile.is_open()) {
        inputFile >> highScore;
        inputFile.close();
    }
}

// Hàm lưu high score vào file
void saveHighScore(int highScore) {
    ofstream outputFile("highscore.dat");
    if (outputFile.is_open()) {
        outputFile << highScore;
        outputFile.close();
    }
}

class Animation
{
public:
    float Frame, speed;       // Frame hiện tại và tốc độ animation
    Sprite sprite;            // Sprite hiển thị
    vector<IntRect> frames;   // Các frame animation

    /* Các phương thức :
      - Khởi tạo animation từ texture
      - Cập nhật frame
      - Kiểm tra kết thúc animation
    */

    Animation() {}

    Animation(Texture& t, int x, int y, int w, int h, int count, float Speed)
    {
        Frame = 0;
        speed = Speed;

        for (int i = 0;i < count;i++)
            frames.push_back(IntRect(x + i * w, y, w, h));

        sprite.setTexture(t);
        sprite.setOrigin(w / 2, h / 2);
        sprite.setTextureRect(frames[0]);
    }


    void update()
    {
        Frame += speed;
        int n = frames.size();
        if (Frame >= n) Frame -= n;
        if (n > 0) sprite.setTextureRect(frames[int(Frame)]);
    }

    bool isEnd()
    {
        return Frame + speed >= frames.size();
    }

};


class Entity
{
public:
    float x, y, dx, dy, R, angle;      // Vị trí và vận tốc, Bán kính va chạm và góc quay
    bool life;                         // Trạng thái sống/chết 
    string name;                       // Tên đối tượng
    Animation anim;                    // Animation

    /* Các phương thức :
     - Thiết lập thông số
     - Cập nhật (ảo để lớp con override)
     - Vẽ đối tượng
    */

    Entity()
    {
        life = 1;
    }

    void settings(Animation& a, int X, int Y, float Angle = 0, int radius = 1)
    {
        anim = a;
        x = X; y = Y;
        angle = Angle;
        R = radius;
    }

    virtual void update() {};

    void draw(RenderWindow& app)
    {
        anim.sprite.setPosition(x, y);
        anim.sprite.setRotation(angle + 90);
        app.draw(anim.sprite);

        CircleShape circle(R);
        circle.setFillColor(Color(255, 0, 0, 170));
        circle.setPosition(x, y);
        circle.setOrigin(R, R);
        //app.draw(circle);
    }

    virtual ~Entity() {};
};

//////////////  Các lớp kế thừa từ Entity  //////////////

// Di chuyển ngẫu nhiên và wrap-around màn hình
class asteroid : public Entity
{
public:
    asteroid()
    {
        dx = rand() % 8 - 4;
        dy = rand() % 8 - 4;
        name = "asteroid";
    }

    void update()
    {
        x += dx;
        y += dy;

        if (x > W) x = 0;  if (x < 0) x = W;
        if (y > H) y = 0;  if (y < 0) y = H;
    }

};

// Bay thẳng theo hướng bắn
class bullet : public Entity
{
public:
    bullet()
    {
        name = "bullet";
    }

    void  update()
    {
        dx = std::cos(angle * DEGTORAD) * 6;
        dy = std::sin(angle * DEGTORAD) * 6;
        // angle+=rand()%7-3;  /*try this*/
        x += dx;
        y += dy;

        if (x > W || x<0 || y>H || y < 0) life = 0;
    }

};

// Lớp người chơi
class player : public Entity
{
public:
    bool thrust;             // Đang tăng tốc

    player()
    {
        name = "player";
    }
    // Xử lý di chuyển với quán tính
    void update()
    {
        if (thrust)
        {
            dx += std::cos(angle * DEGTORAD) * 0.2;
            dy += std::sin(angle * DEGTORAD) * 0.2;
        }
        else
        {
            dx *= 0.99;
            dy *= 0.99;
        }

        int maxSpeed = 15;
        float speed = std::sqrt(dx * dx + dy * dy);
        if (speed > maxSpeed)
        {
            dx *= maxSpeed / speed;
            dy *= maxSpeed / speed;
        }

        x += dx;
        y += dy;

        if (x > W) x = 0; if (x < 0) x = W;
        if (y > H) y = 0; if (y < 0) y = H;
    }

};

////////////////////////////////////////////////////////

//va chạm
bool isCollide(Entity* a, Entity* b)
{
    return (b->x - a->x) * (b->x - a->x) +
        (b->y - a->y) * (b->y - a->y) <
        (a->R + b->R) * (a->R + b->R);
}


int main() {
    srand(time(0));

    loadHighScore(highScore);  // Đọc điểm cao nhất từ file


    RenderWindow app(VideoMode(W, H), "Asteroids!");
    app.setFramerateLimit(60);
        
    // Thêm khởi tạo font - đặt ngay sau khi tạo window
    if (!font.loadFromFile("fonts/arial.ttf")) {
        // Nếu không load được font, tạo font mặc định
        font.loadFromFile("C:/Windows/Fonts/Arial.ttf");
        // Hoặc xử lý lỗi nếu không tìm thấy font
    }

    RectangleShape screenBackground(Vector2f(W, H));
    screenBackground.setFillColor(Color::Black);


    // Load textures
    Texture t1, t2, t3, t4, t5, t6, t7, t8, t9, t10;
    if (!t1.loadFromFile("images/spaceship.png") ||
        !t2.loadFromFile("images/background.jpg") ||
        !t3.loadFromFile("images/explosions/type_C.png") ||
        !t4.loadFromFile("images/rock.png") ||
        !t5.loadFromFile("images/fire_blue.png") ||
        !t6.loadFromFile("images/rock_small.png") ||
        !t7.loadFromFile("images/explosions/type_B.png") ||
        !t8.loadFromFile("images/start_screen.jpg") ||
        !t9.loadFromFile("images/game_over.jpg") || 
        !t10.loadFromFile("images/pause.jpg")){
        return EXIT_FAILURE;
    }

    t1.setSmooth(true);
    t2.setSmooth(true);

    Sprite background(t2);
    Sprite startScreen(t8);
    Sprite gameOverScreen(t9);
    Sprite pauseScreen(t10);

    // Scale sprite
    smartScale(startScreen, W, H, FIT);       // Start screen giữ tỉ lệ
    smartScale(gameOverScreen, W, H, FILL);   // Game over phủ đầy
    smartScale(pauseScreen, W, H, CROP);      // Pause screen crop phần thừa

    // Làm trong suốt pause screen
    pauseScreen.setColor(Color(255, 255, 255, 180));

    Animation sExplosion(t3, 0, 0, 256, 256, 48, 0.5);
    Animation sRock(t4, 0, 0, 64, 64, 16, 0.2);
    Animation sRock_small(t6, 0, 0, 64, 64, 16, 0.2);
    Animation sBullet(t5, 0, 0, 32, 64, 16, 0.8);
    Animation sPlayer(t1, 40, 0, 40, 40, 1, 0);
    Animation sPlayer_go(t1, 40, 40, 40, 40, 1, 0);
    Animation sExplosion_ship(t7, 0, 0, 192, 192, 64, 0.5);


    list<Entity*> entities;
    GameState gameState = START_SCREEN;
    player* p = new player();
  

    while (app.isOpen()) {
        Event event;
        while (app.pollEvent(event)) {
            if (event.type == Event::Closed) {
                saveHighScore(highScore);  // Lưu điểm khi thoát game
                app.close();
            }
            if (event.type == Event::KeyPressed) {
                if (gameState == START_SCREEN && event.key.code == Keyboard::Enter) {
                    // Bắt đầu game mới từ start screen
                    score = 0;
                    entities.clear();
                    asteroidSpawnTimer = 0;

                    p = new player();
                    p->settings(sPlayer, W / 2, H / 2, 0, 20);
                    entities.push_back(p);

                    gameState = PLAYING;
                }
                else if (gameState == GAME_OVER) {
                    if (event.key.code == Keyboard::Enter) {
                        // Restart game
                        score = 0;
                        entities.clear();
                        asteroidSpawnTimer = 0;

                        p = new player();
                        p->settings(sPlayer, W / 2, H / 2, 0, 20);
                        entities.push_back(p);

                        // Tạo lại asteroids
                        for (int i = 0; i < 5; i++) {
                            asteroid* a = new asteroid();
                            a->settings(sRock, rand() % W, rand() % H, rand() % 360, 25);
                            entities.push_back(a);
                        }

                        gameState = PLAYING;
                    }
                    else if (event.key.code == Keyboard::X) {
                        // Về start screen
                        entities.clear();
                        gameState = START_SCREEN;
                    }
                }
                else if (gameState == PAUSED) {
                    if (event.key.code == Keyboard::Enter) {
                        gameState = PLAYING;
                    }
                    else if (event.key.code == Keyboard::X) {
                        entities.clear();
                        gameState = START_SCREEN;
                    }
                }
                else if (gameState == PLAYING && event.key.code == Keyboard::Space) {
                    // Bắn đạn
                    bullet* b = new bullet();
                    b->settings(sBullet, p->x, p->y, p->angle, 10);
                    entities.push_back(b);
                }
            else if (event.key.code == Keyboard::P) {
                // Chỉ xử lý pause khi đang PLAYING hoặc PAUSED
                if (gameState == PLAYING) {
                    gameState = PAUSED;
                }
                else if (gameState == PAUSED) {
                    gameState = PLAYING;
                }
            }
        }
    }
            app.clear();

            // Vẽ nền đen cho các màn hình
            if (gameState == START_SCREEN) {
                app.draw(startScreen);
            }
            else if (gameState == GAME_OVER) {
                app.draw(gameOverScreen);
            }
            else if (gameState == PLAYING) {
                // Game logic
                if (Keyboard::isKeyPressed(Keyboard::Right)) p->angle += 3;
                if (Keyboard::isKeyPressed(Keyboard::Left)) p->angle -= 3;
                if (Keyboard::isKeyPressed(Keyboard::Up)) p->thrust = true;
                else p->thrust = false;

    int currentAsteroids = 0;
    for (auto e : entities) {
        if (e->name == "asteroid") currentAsteroids++;
    }
    
    // Spawn thêm nếu cần
    asteroidSpawnTimer += 1.0f / 60.0f; // Giả sử 60 FPS
    if (asteroidSpawnTimer >= asteroidSpawnInterval && currentAsteroids < maxAsteroids) {
        asteroidSpawnTimer = 0;
        
        // Tạo thiên thạch mới ở rìa màn hình
        Vector2f spawnPos;
        int side = rand() % 4;
        switch (side) {
            case 0: spawnPos = Vector2f(rand() % W, -50); break; // Top
            case 1: spawnPos = Vector2f(W + 50, rand() % H); break; // Right
            case 2: spawnPos = Vector2f(rand() % W, H + 50); break; // Bottom
            case 3: spawnPos = Vector2f(-50, rand() % H); break; // Left
        }
        
        asteroid* a = new asteroid();
        a->settings(sRock, spawnPos.x, spawnPos.y, rand() % 360, 25);
        
        // Bay vào giữa màn hình
        Vector2f direction = Vector2f(W/2, H/2) - spawnPos;
        float length = sqrt(direction.x*direction.x + direction.y*direction.y);
        direction /= length;
        
        float speed = 1.0f + (rand() % 3);
        a->dx = direction.x * speed;
        a->dy = direction.y * speed;
        
        entities.push_back(a);
    }
                // Update entities
                for (auto e : entities) {
                    e->update();
                    e->anim.update();

                    // Tự động xóa explosion khi kết thúc animation
                    if (e->name == "explosion" && e->anim.isEnd()) {
                        e->life = false;
                    }
                }
                // Check collisions
                for (auto a : entities)
                    for (auto b : entities) {
                        if (a->name == "asteroid" && b->name == "bullet") {
                            if (isCollide(a, b)) {
                                a->life = false;
                                b->life = false;

                                // Tính điểm dựa trên kích thước asteroid
                                if (a->R > 20) score += 20;  // Asteroid lớn
                                else if (a->R > 15) score += 50;  // Asteroid trung bình
                                else score += 100;  // Asteroid nhỏ

                                // Cập nhật high score
                                if (score > highScore) {
                                    highScore = score;
                                }

                                // Thêm hiệu ứng explosion
                                Entity* explosion = new Entity();
                                explosion->settings(sExplosion, a->x, a->y);
                                explosion->name = "explosion";
                                entities.push_back(explosion);

                                // Tạo asteroid nhỏ nếu asteroid lớn bị phá hủy
                                if (a->R > 15) {  // Chỉ tạo asteroid nhỏ từ asteroid lớn
                                    for (int i = 0; i < 2; i++) {
                                        Entity* smallAsteroid = new asteroid();
                                        smallAsteroid->settings(sRock_small, a->x, a->y, rand() % 360, 15);
                                        entities.push_back(smallAsteroid);
                                    }
                                }
                            }
                        }
                        if (a->name == "player" && b->name == "asteroid") {
                            if (isCollide(a, b)) {
                                b->life = false;

                                Entity* e = new Entity();
                                e->settings(sExplosion_ship, a->x, a->y);
                                e->name = "explosion";
                                entities.push_back(e);
                                saveHighScore(highScore);  // Lưu điểm khi game over
                                gameState = GAME_OVER;
                                p->life = false;
                            }
                        }
                    }

                // Remove dead entities
                for (auto it = entities.begin(); it != entities.end();) {
                    Entity* e = *it;
                    if (!e->life) { it = entities.erase(it); delete e; }
                    else it++;
                }

                // Draw game
                app.draw(background);
                for (auto e : entities) e->draw(app);

                // Hiển thị scoreboard
                drawScoreboard(app, font, score, highScore, gameState);

            }
            else if (gameState == PAUSED) {
                // Vẽ game hiện tại (đã dừng)
                app.draw(background);
                for (auto e : entities) e->draw(app);

                // Vẽ pause screen phủ lên
                app.draw(pauseScreen);
            }

            app.display();

    }

    // Cleanup
    for (auto e : entities)
        delete e;

    return 0;
}

/*
 Cơ chế spawn thiên thạch
// Mỗi 2 giây kiểm tra số lượng thiên thạch
if (asteroidSpawnTimer >= asteroidSpawnInterval && currentAsteroids < maxAsteroids) {
    // Spawn từ 1 trong 4 cạnh màn hình
    // Bay hướng vào giữa màn hình
    // Tốc độ ngẫu nhiên
}
 Hệ thống tính điểm
// Khi đạn trúng thiên thạch:
- Asteroid lớn: +20 điểm
- Asteroid trung bình: +50 điểm
- Asteroid nhỏ: +100 điểm
- Cập nhật high score nếu cần
 Xử lý va chạm
// Kiểm tra va chạm giữa các entity
bool isCollide(Entity* a, Entity* b) {
    // Khoảng cách < tổng bán kính
}

// Khi va chạm xảy ra:
- Đạn và thiên thạch: nổ, tạo asteroid nhỏ (nếu là lớn)
- Người chơi và thiên thạch: nổ tàu, game over
*/