/* 
   Credit due to the olcConsoleGameEngine:
   https://github.com/OneLoneCoder/videos/blob/master/olcConsoleGameEngine.h

   This code was expanded from a video tutorial that already had some built in physics (especially collision). 
   The video and code are linked as follows:
   https://www.youtube.com/watch?v=LPzyNOHY3A4 
   https://github.com/OneLoneCoder/videos/blob/master/OneLoneCoder_Balls1.cpp
*/

#include <iostream>
#include <string>
using namespace std;

#include "olcConsoleGameEngine.h"

struct sBall
{
    float px, py;
    float vx, vy;
    float ax, ay;
    float default_acc;
    float launch_acc;
    float radius;
    float mass;
    float co_rest;
    int id;
};

class CirclePhysics : public olcConsoleGameEngine
{
public:
    CirclePhysics()
    {
        m_sAppName = L"Circle Physics";
    }

private:
    vector<pair<float, float>> modelCircle;
    vector<sBall> vecBalls;
    sBall* pSelectedBall = nullptr;  // Pointer to be used for the currently selected ball 
    sBall* nonSelectedBall = nullptr;  // Pointer to be used for the non-selected ball 

    void AddBall(float x, float y, float r = 5.0f)
    {
        sBall b;
        b.px = x; b.py = y;
        b.vx = 0; b.vy = 1.0f;
        b.ax = 0; b.ay = 15.0f;
        b.default_acc = 15.0f;  // default acceleration (gravity)
        b.launch_acc = -15.0f;  // acceleration imparted by missile/rocket thrust (to be SUMMED with default acceleration, not replacing)
        b.radius = r;
        b.mass = r * 10.0f;
        b.co_rest = 0.6f;

        b.id = vecBalls.size();
        vecBalls.emplace_back(b);
    }

public:
    bool OnUserCreate()
    {
        //Define Circle Model
        modelCircle.push_back({ 0.0f, 0.0f });
        int nPoints = 20;
        for (int i = 0; i < nPoints; i++)
            modelCircle.push_back({ cosf(i / (float)(nPoints - 1) * 2.0f * 3.14159f) , sinf(i / (float)(nPoints - 1) * 2.0f * 3.14159f) });

        float fDefaultRad = 5.0f;

        AddBall(ScreenWidth() * 0.1f, ScreenHeight() * 0.8f, fDefaultRad);  
        AddBall(ScreenWidth() * 0.9f, ScreenHeight() * 0.8f, fDefaultRad);
        
        return true;
    }

    bool OnUserUpdate(float fElapsedTime)
    {
        // DoCirclesOverlap: check if distance between the center of circles 1 and 2 is less than sum of radii (min distance without overlap)
        auto DoCirclesOverlap = [](float x1, float y1, float r1, float x2, float y2, float r2)
        {
            return fabs((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)) <= (r1 + r2) * (r1 + r2);
        };
        
        // Distance check 
        auto IsPointInCircle = [](float x1, float y1, float r1, float px, float py)
        {
            return fabs((x1 - px) * (x1 - px) + (y1 - py) * (y1 - py)) < (r1 * r1);
        };

        // Identify selected and non-selected balls with either mouse button
        if (m_mouse[0].bPressed || m_mouse[1].bPressed || m_mouse[2].bPressed)
        {
            pSelectedBall = nullptr;
            nonSelectedBall = nullptr;
            for (auto &ball : vecBalls)
            {
                // Check if mouse button click occurred inside the circle radius 
                if (IsPointInCircle(ball.px, ball.py, ball.radius, m_mousePosX, m_mousePosY))
                {
                    pSelectedBall = &ball;
                    //break;
                }
                // Check the opposite criteria; if clicked outside, then this is the non-selected object
                if (!IsPointInCircle(ball.px, ball.py, ball.radius, m_mousePosX, m_mousePosY))
                {
                    nonSelectedBall = &ball;
                    //break;
                }
            }
        }

        if (m_mouse[0].bHeld)
        {
            if (pSelectedBall != nullptr)
            {
                if (nonSelectedBall != nullptr) 
                {
                    pSelectedBall->vx = 0; // Without clamping the velocity, clicking and dragging will cause the selected ball to accumulate velocity
                    pSelectedBall->vy = 0; 
                    pSelectedBall->px = m_mousePosX;
                    if (m_mousePosY < ScreenHeight() - 10 - pSelectedBall->radius) pSelectedBall->py = m_mousePosY;
                }
            }
        }
        
        if (m_mouse[0].bReleased)
        {
            pSelectedBall = nullptr;            
            nonSelectedBall = nullptr;
        }

        if (m_mouse[1].bReleased)
        {
            if (pSelectedBall != nullptr)
            {
                // Apply Launch Velocity (where 5 is just a scaling factor of the distance between the ball and my mouse position)
                pSelectedBall->vx = 5.0f * ((pSelectedBall->px) - (float)m_mousePosX);
                pSelectedBall->vy = 5.0f * ((pSelectedBall->py) - (float)m_mousePosY);

                // Apply Launch Acceleration (along direction of vy & vx)
                float v_hyp = sqrtf(pSelectedBall->vx * pSelectedBall->vx + pSelectedBall->vy * pSelectedBall->vy);
                pSelectedBall->ay = pSelectedBall->launch_acc * -pSelectedBall->vy / v_hyp;
                pSelectedBall->ax = pSelectedBall->launch_acc * pSelectedBall->vx / v_hyp;
            }

            pSelectedBall = nullptr;
            nonSelectedBall = nullptr;

        }

        if (m_mouse[2].bReleased)
        {
            // The time to interception must be defined in order to resolve the intercepting velocity
            int time_inc = 2;
            if (pSelectedBall != nullptr)
            {
                // Apply Interception Velocity
                pSelectedBall->vx = (nonSelectedBall->vx * time_inc + nonSelectedBall->px - pSelectedBall->px) / time_inc;
                pSelectedBall->vy = (nonSelectedBall->vy * time_inc + nonSelectedBall->py - pSelectedBall->py) / time_inc;
            }

            pSelectedBall = nullptr;
            nonSelectedBall = nullptr;
        }

        vector<pair<sBall*, sBall*>> vecCollidingPairs;

        // Update Ball Positions
        for (int i = 0; i < 2; i++)
        {
            for (auto& ball : vecBalls)
            {
                int floor = ScreenHeight() - 10;

                // These two lines preserve the functionality of 'gravity' after clamping to zero in the following if statement
                float ax_temp = ball.ax;
                float ay_temp = ball.ay;

                // Clamp velocity near zero
                if (fabs(ball.vy) < 0.1f) 
                {
                    if (ball.py + ball.radius >= floor)
                    {
                        ball.vy = 0;
                        ball.ax = -ball.vx * ball.co_rest;
                        ball.ay = 0;
                    }
                }

                if (ball.ay != ball.default_acc)
                {
                    if (ball.py < ScreenHeight() * 0.6)
                    {
                        ball.ay = ball.default_acc;
                        ay_temp = ball.ay;
                    }
                }
                if (ball.ax != 0)
                {
                    if (ball.py < ScreenHeight() * 0.6)
                    {
                        ball.ax = 0;
                        ax_temp = ball.ax;
                    }
                }

                // Update Ball Physics
                ball.vx += ball.ax * fElapsedTime;
                ball.vy += ball.ay * fElapsedTime;
                ball.px += ball.vx * fElapsedTime;
                ball.py += ball.vy * fElapsedTime;

                // Bounce on border frames (formerly: Wrap the balls around screen)
                if (ball.px - ball.radius < 0)
                {
                    if (ball.vx < 0) ball.vx = -ball.vx * ball.co_rest;
                }

                if (ball.px + ball.radius >= ScreenWidth())
                {
                    if (ball.vx > 0) ball.vx = -ball.vx * ball.co_rest;
                }

                if (ball.py - ball.radius < 0)
                {
                    if (ball.vy < 0) ball.vy = -ball.vy * ball.co_rest;
                }

                if (ball.py + ball.radius >= floor)
                {
                    if (ball.vy > 0)
                    {
                        ball.vy = 0; 
                    }
                    if (ball.vy == 0)
                    {
                        ball.py = floor - ball.radius;
                    }
                }

                ball.ax = ax_temp;
                ball.ay = ay_temp;
            }
        }
        
        for (auto &ball : vecBalls)
        {
            for (auto &target : vecBalls)
            {
                if (ball.id != target.id)
                {
                    if (DoCirclesOverlap(ball.px, ball.py, ball.radius, target.px, target.py, target.radius))
                    {
                        // Collision has Occured
                        vecCollidingPairs.push_back({ &ball, &target });
                        
                        // Distance between ball centers
                        float fDistance = sqrtf((ball.px - target.px) * (ball.px - target.px) + (ball.py - target.py) * (ball.py - target.py));

                        float fOverlap = 0.5f * (fDistance - ball.radius - target.radius);

                        // Displace Current Ball away from collision
                        ball.px -= fOverlap * (ball.px - target.px) / fDistance;
                        ball.py -= fOverlap * (ball.py - target.py) / fDistance;

                        // Displace Target Ball away from collision
                        target.px += fOverlap * (ball.px - target.px) / fDistance;
                        target.py += fOverlap * (ball.py - target.py) / fDistance;
                    }
                }
            }
        }
        
        // Dynamic Collisions
        for (auto c : vecCollidingPairs)
        {
            sBall *b1 = c.first;
            sBall *b2 = c.second;

            // Distance Between Balls
            float fDistance = sqrtf((b1->px - b2->px) * (b1->px - b2->px) + (b1->py - b2->py) * (b1->py - b2->py));

            // Normal
            float nx = (b2->px - b1->px) / fDistance;
            float ny = (b2->py - b1->py) / fDistance;

            // Tangent
            float tx = -ny;
            float ty = nx;

            // Dot Product Tangent
            float dpTan1 = b1->vx * tx + b1->vy * ty;
            float dpTan2 = b2->vx * tx + b2->vy * ty;

            // Dot Product Normal
            float dpNorm1 = b1->vx * nx + b1->vy * ny;
            float dpNorm2 = b2->vx * nx + b2->vy * ny;

            // Conservation of momentum in 1D
            float m1 = (dpNorm1 * (b1->mass - b2->mass) + 2.0f * b2->mass * dpNorm2) / (b1->mass + b2->mass);
            float m2 = (dpNorm2 * (b2->mass - b1->mass) + 2.0f * b1->mass * dpNorm1) / (b1->mass + b2->mass);

            // Update ball velocities
            b1->vx = tx * dpTan1 + nx * m1;
            b1->vy = ty * dpTan1 + ny * m1 + b1->ay * fElapsedTime;
            b2->vx = tx * dpTan2 + nx * m2;
            b2->vy = ty * dpTan2 + ny * m2 + b2->ay * fElapsedTime;
        }

        // Clear Screen
        Fill(0, 0, ScreenWidth(), ScreenHeight(), ' ');

        // Draw Map Floor 
        int floor = ScreenHeight() - 10;
        for (int y = 0; y < 10; y++)
            for (int x = 0; x < ScreenWidth(); x++)
                Draw(x, floor + y, PIXEL_SOLID, FG_DARK_GREEN);

        // Draw Balls
        for (auto ball : vecBalls)
        {
            if (ball.ay != ball.default_acc)
            {
                DrawWireFrameModel(modelCircle, ball.px, ball.py, atan2f(ball.vy, ball.vx), ball.radius, FG_CYAN);
            }
            else
            {
                DrawWireFrameModel(modelCircle, ball.px, ball.py, atan2f(ball.vy, ball.vx), ball.radius, FG_WHITE);
            }

            if (ball.py < floor - ball.radius)
            {   
                for (int t = 1; t < 300; t+=20)
                {
                    float y_traj = ball.py + ball.vy * (fElapsedTime * t) + 0.5f * ball.ay * (fElapsedTime * t) * (fElapsedTime * t);
                    if (y_traj < floor)
                    {
                        float x_traj = ball.px + ball.vx * (fElapsedTime * t);
                        Draw(x_traj, y_traj, PIXEL_SOLID, FG_RED);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
   
        for (auto c : vecCollidingPairs)
        {
            // Impact Line
            DrawLine(c.first->px, c.first->py, c.second->px, c.second->py, PIXEL_SOLID, FG_RED);
        }

        if (pSelectedBall != nullptr) 
        {
            // Draw Cue
            DrawLine(pSelectedBall->px, pSelectedBall->py, m_mousePosX, m_mousePosY, PIXEL_SOLID, FG_BLUE);
        }
        
        if (nonSelectedBall != nullptr)
        {
            // Draw cue
            DrawLine(nonSelectedBall->px, nonSelectedBall->py, m_mousePosX, m_mousePosY, PIXEL_SOLID, FG_GREEN);
        }
        
        return true;
    }
};

int main()
{
    CirclePhysics game;
    if (game.ConstructConsole(200, 120, 8, 8))
        game.Start();
    else
        wcout << L"Could not construct console" << endl;

    return 0;
}
