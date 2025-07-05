#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <cstring> // memcpy
#include <future> // std::async

using namespace std;

const short SIZE = 15;
short MAX_DEPTH = 2;
const int MAX_THREADS = thread::hardware_concurrency() * 2;
const int INF = 1e9;

short board[SIZE][SIZE] = {0};
short dx[] = {1, 0, 1, 1};
short dy[] = {0, 1, 1, -1};

short myPlayer = 2; // AI
short humanPlayer = 1;

mutex mtx;
int globalBestScore;
pair<char,char> globalBestMove;

vector<pair<short, short>> getValidMoves();

void printBoard()
{
    cout << "  ";
    for (short i = 0; i < SIZE; i++)
        cout << i % 10 << " ";
    cout << "\n";
    for (short i = 0; i < SIZE; i++)
    {
        cout << i % 10 << " ";
        for (short j = 0; j < SIZE; j++)
        {
            if (board[i][j] == humanPlayer)
                cout << "X ";
            else if (board[i][j] == myPlayer)
                cout << "O ";
            else
                cout << ". ";
        }
        cout << "\n";
    }
}

bool isValidMove(short x, short y)
{
    return x >= 0 && x < SIZE && y >= 0 && y < SIZE && board[x][y] == 0;
}

bool checkWin(short x, short y, short player)
{
    for (short dir = 0; dir < 4; dir++)
    {
        short count = 1;
        for (short step = 1; step < 5; step++)
        {
            short nx = x + step * dx[dir], ny = y + step * dy[dir];
            if (nx < 0 || ny < 0 || nx >= SIZE || ny >= SIZE || board[nx][ny] != player)
                break;
            count++;
        }
        for (short step = 1; step < 5; step++)
        {
            short nx = x - step * dx[dir], ny = y - step * dy[dir];
            if (nx < 0 || ny < 0 || nx >= SIZE || ny >= SIZE || board[nx][ny] != player)
                break;
            count++;
        }
        if (count >= 5)
            return true;
    }
    return false;
}

// Phiên bản checkWin nhận board riêng cho đa luồng
bool checkWin_thread(short x, short y, short player, short localBoard[SIZE][SIZE])
{
    for (short dir = 0; dir < 4; dir++)
    {
        short count = 1;
        for (short step = 1; step < 5; step++)
        {
            short nx = x + step * dx[dir], ny = y + step * dy[dir];
            if (nx < 0 || ny < 0 || nx >= SIZE || ny >= SIZE || localBoard[nx][ny] != player)
                break;
            count++;
        }
        for (short step = 1; step < 5; step++)
        {
            short nx = x - step * dx[dir], ny = y - step * dy[dir];
            if (nx < 0 || ny < 0 || nx >= SIZE || ny >= SIZE || localBoard[nx][ny] != player)
                break;
            count++;
        }
        if (count >= 5)
            return true;
    }
    return false;
}

// Đánh giá một dãy liên tiếp
int evaluateLine(short count, short openEnds, short player)
{
    if (count >= 5)
        return (player == myPlayer) ? 100000 : -100000;

    int score = 0;
    if (openEnds == 0)
        return 0; // Bị chặn 2 đầu

    if (player == myPlayer)
    {
        switch (count)
        {
        case 4:
            score = (openEnds == 2) ? 10000 : 1000;
            break;
        case 3:
            score = (openEnds == 2) ? 500 : 100;
            break;
        case 2:
            score = (openEnds == 2) ? 50 : 10;
            break;
        case 1:
            score = (openEnds == 2) ? 5 : 1;
            break;
        }
    }
    else
    {
        switch (count)
        {
        case 4:
            score = (openEnds == 2) ? -50000 : -5000;
            break; // Phải chặn ngay
        case 3:
            score = (openEnds == 2) ? -2000 : -200;
            break;
        case 2:
            score = (openEnds == 2) ? -100 : -20;
            break;
        case 1:
            score = (openEnds == 2) ? -10 : -2;
            break;
        }
    }
    return score;
}

// Đánh giá một vị trí theo 4 hướng
int evaluatePosition(short x, short y, short player)
{
    int totalScore = 0;

    for (short dir = 0; dir < 4; dir++)
    {
        short count = 1;
        short leftBlocked = 0, rightBlocked = 0;

        for (short step = 1; step < 5; step++)
        {
            short nx = x + step * dx[dir];
            short ny = y + step * dy[dir];
            if (nx < 0 || ny < 0 || nx >= SIZE || ny >= SIZE)
            {
                rightBlocked = 1;
                break;
            }
            if (board[nx][ny] == player)
            {
                count++;
            }
            else if (board[nx][ny] == 0)
            {
                break;
            }
            else
            {
                rightBlocked = 1;
                break;
            }
        }

        for (short step = 1; step < 5; step++)
        {
            short nx = x - step * dx[dir];
            short ny = y - step * dy[dir];
            if (nx < 0 || ny < 0 || nx >= SIZE || ny >= SIZE)
            {
                leftBlocked = 1;
                break;
            }
            if (board[nx][ny] == player)
            {
                count++;
            }
            else if (board[nx][ny] == 0)
            {
                break;
            }
            else
            {
                leftBlocked = 1;
                break;
            }
        }

        short openEnds = 2 - leftBlocked - rightBlocked;
        totalScore += evaluateLine(count, openEnds, player);
    }

    return totalScore;
}

// Phiên bản evaluatePosition nhận board riêng cho đa luồng
int evaluatePosition_thread(short x, short y, short player, short localBoard[SIZE][SIZE])
{
    short totalScore = 0;

    for (short dir = 0; dir < 4; dir++)
    {
        short count = 1;
        short leftBlocked = 0, rightBlocked = 0;

        for (short step = 1; step < 5; step++)
        {
            short nx = x + step * dx[dir];
            short ny = y + step * dy[dir];
            if (nx < 0 || ny < 0 || nx >= SIZE || ny >= SIZE)
            {
                rightBlocked = 1;
                break;
            }
            if (localBoard[nx][ny] == player)
            {
                count++;
            }
            else if (localBoard[nx][ny] == 0)
            {
                break;
            }
            else
            {
                rightBlocked = 1;
                break;
            }
        }

        for (short step = 1; step < 5; step++)
        {
            short nx = x - step * dx[dir];
            short ny = y - step * dy[dir];
            if (nx < 0 || ny < 0 || nx >= SIZE || ny >= SIZE)
            {
                leftBlocked = 1;
                break;
            }
            if (localBoard[nx][ny] == player)
            {
                count++;
            }
            else if (localBoard[nx][ny] == 0)
            {
                break;
            }
            else
            {
                leftBlocked = 1;
                break;
            }
        }

        short openEnds = 2 - leftBlocked - rightBlocked;
        totalScore += evaluateLine(count, openEnds, player);
    }

    return totalScore;
}

// Hàm đánh giá toàn bộ bàn cờ
int evaluateBoard()
{
    int score = 0;

    for (short i = 0; i < SIZE; i++)
    {
        for (short j = 0; j < SIZE; j++)
        {
            if (board[i][j] != 0)
            {
                score += evaluatePosition(i, j, board[i][j]);
            }
        }
    }

    return score;
}

// Phiên bản evaluateBoard nhận board riêng cho đa luồng
int evaluateBoard_thread(short localBoard[SIZE][SIZE])
{
    int score = 0;

    for (short i = 0; i < SIZE; i++)
    {
        for (short j = 0; j < SIZE; j++)
        {
            if (localBoard[i][j] != 0)
            {
                score += evaluatePosition_thread(i, j, localBoard[i][j], localBoard);
            }
        }
    }
    return score;
}

// Kiểm tra nước đi tức thắng hoặc phải chặn
pair<short, short> findCriticalMove()
{
    vector<pair<short, short>> moves = getValidMoves();

    // Kiểm tra nước thắng
    for (auto m : moves)
    {
        board[m.first][m.second] = myPlayer;
        if (checkWin(m.first, m.second, myPlayer))
        {
            board[m.first][m.second] = 0;
            return m;
        }
        board[m.first][m.second] = 0;
    }

    // Kiểm tra nước phải chặn
    for (auto m : moves)
    {
        board[m.first][m.second] = humanPlayer;
        if (checkWin(m.first, m.second, humanPlayer))
        {
            board[m.first][m.second] = 0;
            return m;
        }
        board[m.first][m.second] = 0;
    }

    return {-1, -1};
}

// Phiên bản getValidMoves nhận board riêng cho đa luồng
vector<pair<short, short>> getValidMoves_thread(short localBoard[SIZE][SIZE])
{
    vector<pair<short, short>> moves;
    const short RANGE = 2;
    bool visited[SIZE][SIZE] = {false};

    for (short i = 0; i < SIZE; i++)
    {
        for (short j = 0; j < SIZE; j++)
        {
            if (localBoard[i][j] != 0)
            {
                for (short x = max(0, i - RANGE); x <= min(SIZE - 1, i + RANGE); x++)
                {
                    for (short y = max(0, j - RANGE); y <= min(SIZE - 1, j + RANGE); y++)
                    {
                        if (localBoard[x][y] == 0 && !visited[x][y])
                        {
                            moves.push_back({x, y});
                            visited[x][y] = true;
                        }
                    }
                }
            }
        }
    }

    if (moves.empty())
    {
        moves.push_back({SIZE / 2, SIZE / 2});
    }

    sort(moves.begin(), moves.end(), [&](const pair<short, short> &a, const pair<short, short> &b)
         {
        localBoard[a.first][a.second] = myPlayer;
        int scoreA = evaluatePosition_thread(a.first, a.second, myPlayer, localBoard);
        localBoard[a.first][a.second] = 0;
        
        localBoard[b.first][b.second] = myPlayer;
        int scoreB = evaluatePosition_thread(b.first, b.second, myPlayer, localBoard);
        localBoard[b.first][b.second] = 0;
        
        return scoreA > scoreB; });

    return moves;
}

vector<pair<short, short>> getValidMoves()
{
    vector<pair<short, short>> moves;
    const short RANGE = 2;
    bool visited[SIZE][SIZE] = {false};

    for (short i = 0; i < SIZE; i++)
    {
        for (short j = 0; j < SIZE; j++)
        {
            if (board[i][j] != 0)
            {
                for (short x = max(0, i - RANGE); x <= min(SIZE - 1, i + RANGE); x++)
                {
                    for (short y = max(0, j - RANGE); y <= min(SIZE - 1, j + RANGE); y++)
                    {
                        if (board[x][y] == 0 && !visited[x][y])
                        {
                            moves.push_back({x, y});
                            visited[x][y] = true;
                        }
                    }
                }
            }
        }
    }

    if (moves.empty())
    {
        moves.push_back({SIZE / 2, SIZE / 2});
    }

    // Sắp xếp nước đi theo điểm đánh giá (để cắt tỉa tốt hơn)
    sort(moves.begin(), moves.end(), [](const pair<short, short> &a, const pair<short, short> &b)
         {
        board[a.first][a.second] = myPlayer;
        int scoreA = evaluatePosition_thread(a.first, a.second, myPlayer, board);
        board[a.first][a.second] = 0;
        
        board[b.first][b.second] = myPlayer;
        int scoreB = evaluatePosition_thread(b.first, b.second, myPlayer, board);
        board[b.first][b.second] = 0;
        
        return scoreA > scoreB; });

    return moves;
}

// Phiên bản minimax nhận board riêng cho đa luồng
int minimax_thread(short depth, int alpha, int beta, bool isMax, short localBoard[SIZE][SIZE])
{
    if (depth == 0)
    {
        return evaluateBoard_thread(localBoard);
    }

    vector<pair<short, short>> moves = getValidMoves_thread(localBoard);
    if (moves.empty())
        return 0;

    if (isMax)
    {
        int maxEval = -INF;
        for (auto m : moves)
        {
            localBoard[m.first][m.second] = myPlayer;

            if (checkWin_thread(m.first, m.second, myPlayer, localBoard))
            {
                localBoard[m.first][m.second] = 0;
                return 100000;
            }

            int eval = minimax_thread(depth - 1, alpha, beta, false, localBoard);
            localBoard[m.first][m.second] = 0;

            maxEval = max(maxEval, eval);
            alpha = max(alpha, eval);
            if (beta <= alpha)
                break;
        }
        return maxEval;
    }
    else
    {
        int minEval = INF;
        for (auto m : moves)
        {
            localBoard[m.first][m.second] = humanPlayer;

            if (checkWin_thread(m.first, m.second, humanPlayer, localBoard))
            {
                localBoard[m.first][m.second] = 0;
                return -100000;
            }

            int eval = minimax_thread(depth - 1, alpha, beta, true, localBoard);
            localBoard[m.first][m.second] = 0;

            minEval = min(minEval, eval);
            beta = min(beta, eval);
            if (beta <= alpha)
                break;
        }
        return minEval;
    }
}

// pair<short, short> findBestMove()
// {
//     // Kiểm tra nước đi quan trọng trước
//     pair<short, short> critical = findCriticalMove();
//     if (critical.first != -1)
//     {
//         return critical;
//     }

//     globalBestScore = -INF;
//     globalBestMove = {-1, -1};
//     vector<pair<short, short>> moves = getValidMoves();

//     vector<thread> threads;

//     for (auto m : moves)
//     {
//         threads.emplace_back([m]() {
//             short localBoard[SIZE][SIZE];
//             memcpy(localBoard, board, sizeof(board));
//             localBoard[m.first][m.second] = myPlayer;

//             int score = minimax_thread(MAX_DEPTH - 1, -INF, INF, false, localBoard);

//             lock_guard<mutex> lock(mtx);
//             if (score > globalBestScore)
//             {
//                 globalBestScore = score;
//                 globalBestMove = m;
//             }
//         });
//     }

//     for (auto& t : threads)
//         t.join();

//     return globalBestMove;
// }

#include <future> // dùng std::future

pair<short, short> findBestMove()
{
    pair<short, short> critical = findCriticalMove();
    if (critical.first != -1)
        return critical;

    globalBestScore = -INF;
    globalBestMove = {-1, -1};
    vector<pair<short, short>> moves = getValidMoves();

    vector<future<void>> futures;

    for (size_t i = 0; i < moves.size(); i++)
    {
        auto m = moves[i];
        futures.push_back(async(launch::async, [m]() {
            short localBoard[SIZE][SIZE];
            memcpy(localBoard, board, sizeof(board));
            localBoard[m.first][m.second] = myPlayer;

            int score = minimax_thread(MAX_DEPTH - 1, -INF, INF, false, localBoard);

            lock_guard<mutex> lock(mtx);
            if (score > globalBestScore)
            {
                globalBestScore = score;
                globalBestMove = m;
            }
        }));

        // Nếu đã tạo đủ 8 luồng thì chờ xong rồi mới tạo tiếp
        if (futures.size() >= MAX_THREADS || i == moves.size() - 1)
        {
            for (auto& f : futures)
                f.get(); // chờ luồng xong

            futures.clear(); // reset cho batch tiếp theo
        }
    }

    return globalBestMove;
}

void level() {
    cout << "Chon muc do choi (1-3):\n";
    cout << "1. De\n";
    cout << "2. Trung binh\n";
    cout << "3. Kho\n";
    short choice;
    cin >> choice;

    switch (choice) {
        case 1:
            // Dễ: Giữ nguyên độ sâu
            break;
        case 2:
            // Trung bình: Giảm độ sâu xuống 3
            MAX_DEPTH = 3;
            break;
        case 3:
            // Khó: Tăng độ sâu lên 5
            MAX_DEPTH = 5;
            break;
        default:
            cout << "Lua chon khong hop le, su dung muc do de.\n";
            break;
    }
}

int main()
{
    cout << "Game Caro 15x15: Nguoi (X) vs May (O)\n";
    cout << "May da duoc nang cap voi AI thong minh hon!\n";
    level(); // Chọn mức độ chơi

    while (true)
    {
        printBoard();
        short x, y;
        cout << "Nhap nuoc di cua ban (x y): ";
        cin >> x >> y;

        if (!isValidMove(x, y))
        {
            cout << "Nuoc di khong hop le, thu lai.\n";
            continue;
        }

        board[x][y] = humanPlayer;
        printBoard();
        if (checkWin(x, y, humanPlayer))
        {
            printBoard();
            cout << "Ban thang!\n";
            break;
        }

        cout << "May dang suy nghi...\n";
        pair<short, short> aiMove = findBestMove();
        if (aiMove.first == -1)
        {
            printBoard();
            cout << "Hoa!\n";
            break;
        }

        board[aiMove.first][aiMove.second] = myPlayer;
        cout << "May danh: (" << aiMove.first << ", " << aiMove.second << ")\n";

        if (checkWin(aiMove.first, aiMove.second, myPlayer))
        {
            printBoard();
            cout << "May thang!\n";
            break;
        }
    }
    return 0;
}
