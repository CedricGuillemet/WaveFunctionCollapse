// wfc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <vector>
#include <assert.h>
#include <chrono>
#include <iostream>

struct Coord
{
    int x, y;
};


struct Tile
{
    char mBitmap;
    char mKeys[4];
};

// +x
// +y
// -x
// -y
/*
static const int tileCount = 10;
Tile mTiles[tileCount] = {
    {'X',{1,1,1,1}},
{'/',{1,0,0,1}},
{'\\',{0,0,1,1}},
{'|',{1,0,0,0}},
{'|',{0,0,1,0}},
{'-',{0,0,0,1}},
{'-',{0,1,0,0}},
{'/',{0,1,1,0}},
{'\\',{1,1,0,0}},
{' ',{0,0,0,0}}
};

/*
static const int tileCount = 3;
Tile mTiles[tileCount] = {
    {' ',{2,2,2,2}},
    {'X',{1,1,1,1}},
{'.',{3,3,3,3}},

};
*/

static const int tileCount = 6;
Tile mTiles[tileCount] = {
    {' ',{2,2,2,2}},
    {'X',{1,1,1,1}},
    {'-',{1,2,1,1}},
    {'-',{1,1,1,2}},
    {'|',{2,1,1,1}},
    {'|',{1,1,2,1}},
};

int GetAngle(Coord dir)
{
    if (dir.x == 1)
        return 0;
    if (dir.y == 1)
        return 1;
    if (dir.x == -1)
        return 2;
    if (dir.y == -1)
        return 3;
    assert(0);
    return -1;
}

int GetHook(Coord dir)
{
    if (dir.x == 1)
        return 2;
    if (dir.y == 1)
        return 3;
    if (dir.x == -1)
        return 0;
    if (dir.y == -1)
        return 1;
    assert(0);
    return -1;
}

bool TileCompatible(int tileIndex1, int tileIndex2, Coord dir)
{
    int key1 = mTiles[tileIndex1].mKeys[GetAngle(dir)];
    int key2 = mTiles[tileIndex2].mKeys[GetHook(dir)];
    return (key1 & key2) != 0;
}

struct Model
{
    Model(int width, int height)
        : mWidth(width), mHeight(height)
    {
        mCoef.resize(width*height * tileCount, true);
        /*for (auto&c : mCoef)
        {
            c.resize(tileCount, true);
        }
        */
        mSumCoef.resize(width*height, tileCount);
        mTotalSum = width * height * tileCount;
    }

    int GetTileAtIndex(Coord coord)
    {
        int idx = (coord.y * mWidth + coord.x) * tileCount;
        int res = -1;
        for (int i = 0; i < tileCount; i++)
        {
            if (mCoef[idx + i])
            {
                assert(res == -1);
                res = i;
            }
        }
        return res;
    }

    Coord GetMinEntropy()
    {
        int minEntropy = INT_MAX;
        static std::vector<Coord> minEntropyCoords;
        minEntropyCoords.clear();
        for (int y = 0; y < mHeight; y++)
        {
            for (int x = 0; x < mWidth; x++)
            {
                int coef = mSumCoef[y * mWidth + x];
                if (coef == 1)
                    continue;
                if (coef < minEntropy)
                {
                    minEntropy = coef;
                    minEntropyCoords.clear();
                }
                if (coef == minEntropy)
                    minEntropyCoords.push_back({ x, y });
            }
        }
        assert(!minEntropyCoords.empty());
        return minEntropyCoords[rand() % minEntropyCoords.size()];
    }
    bool IsFullyCollapsed()
    {
        return mTotalSum == (mWidth * mHeight);
    }
    void Collapse(Coord coord, int tileIndex)
    {
        int idx = (coord.y * mWidth + coord.x)*tileCount;
        for (unsigned int i = 0; i < tileCount; i++)
        {
            if (mCoef[idx + i])
                mTotalSum--;
            mCoef[idx + i] = 0;
        }
        mCoef[idx + tileIndex] = 1;
        mTotalSum++;
        mSumCoef[coord.y * mWidth + coord.x] = 1;
    }
    void Collapse(Coord coord)
    {
        int potentials[tileCount];
        int potentialIndex = 0;
        int idx = (coord.y * mWidth + coord.x)*tileCount;
        //auto & v = mCoef[coord.y * mWidth + coord.x];
        int cnt = 0;
        for (unsigned int i = 0; i < tileCount; i++)
        {
            if (mCoef[idx + i])
                potentials[potentialIndex++] = i;
        }
        assert(potentialIndex);
        static int rd = 0;
        int selected = potentials[rand() % potentialIndex];
        Collapse(coord, selected);
    }
    int GetPossibleTiles(Coord coord, int* possibleTiles)
    {
        int res = 0;
        int idx = (coord.y * mWidth + coord.x)*tileCount;
        //auto & v = mCoef[coord.y * mWidth + coord.x];
        for (unsigned int i = 0; i < tileCount; i++)
        {
            if (mCoef[idx + i])
                possibleTiles[res++] = i;
        }
        return res;
    }
    void GetValidDirs(Coord coord, Coord *dest, int& coordCount)
    {
        if (coord.x < (mWidth-1))
            dest[coordCount++] = {1, 0 };
        if (coord.y < (mHeight - 1))
            dest[coordCount++] = { 0, 1 };
        if (coord.x > 0)
            dest[coordCount++] = { -1, 0 };
        if (coord.y > 0)
            dest[coordCount++] = {0, -1};
    }
    void Constrain(Coord coord, int tileIndex)
    {
        int idx = (coord.y * mWidth + coord.x)*tileCount;
        //auto & v = mCoef[coord.y * mWidth + coord.x];
        assert(mCoef[idx + tileIndex]);
        mCoef[idx + tileIndex] = 0;
        mSumCoef[coord.y * mWidth + coord.x] --;
        mTotalSum--;
    }
    void Propagate(Coord coord)
    {
        static std::vector<Coord> coords;
        coords.clear();
        coords.push_back(coord);
        while (coords.size())
        {
            Coord currentCoord = coords.back();
            coords.pop_back();
            
            int curPossibleTiles[tileCount];
            int curPossibleTileCount = GetPossibleTiles(currentCoord, curPossibleTiles);

            Coord validDirs[4];
            int validDirCount = 0;
            GetValidDirs(currentCoord, validDirs, validDirCount);
            for (int d = 0 ; d < validDirCount ; d++)
            {
                Coord dir = validDirs[d];
                Coord otherCoord = { currentCoord.x + dir.x, currentCoord.y + dir.y };
                int otherPossibleTiles[tileCount];
                int otherPossibleTileCount = GetPossibleTiles(otherCoord, otherPossibleTiles);
                for (int otherTileIndex = 0;otherTileIndex< otherPossibleTileCount;otherTileIndex++)
                {
                    int otherTile = otherPossibleTiles[otherTileIndex];
                    bool tileCompatible = false;
                    for (int curTileIndex = 0;curTileIndex< curPossibleTileCount;curTileIndex++)
                    {
                        int curTile = curPossibleTiles[curTileIndex];
                        tileCompatible |= TileCompatible(curTile, otherTile, dir);
                    }
                    if (!tileCompatible)
                    {
                        Constrain(otherCoord, otherTile);
                        coords.push_back(otherCoord);
                    }
                }
            }
        }
    }

    void Run()
    {
        while (!IsFullyCollapsed())
        {
            Coord coord = GetMinEntropy();
            Collapse(coord);
            Propagate(coord);
        }
    }

    void Dump()
    {
        for (int y = 0; y < mHeight; y++)
        {
            for (int x = 0; x < mWidth; x++)
            {
                int tileIndex = GetTileAtIndex(Coord{ x,y });
                printf("%c", mTiles[tileIndex].mBitmap);
            }
            printf("\n");
        }
    }

    int mWidth, mHeight;
    std::vector<bool> mCoef;
    std::vector<unsigned short> mSumCoef;
    unsigned int mTotalSum;
};


int main()
{
    srand(57784);
    
    int runCount = 5000;
    auto t1 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < runCount; i++)
    {
        Model model(100, 25);
        model.Run();
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
    
    printf("Time per solve : %2.4f seconds\n", float(time_span.count()) / float(runCount));
        

    Model model(100, 25);
    model.Run();
    model.Dump();
    
}

