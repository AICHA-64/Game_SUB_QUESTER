// ----------------------------------------------------
// ルート検索の仕組み [route_search.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-12-15
// Version: 1.0
// ----------------------------------------------------
#ifndef ROUTE_SEARCH_H
#define ROUTE_SEARCH_H

void RouteSearch_Initialize();
void RouteSearch_Finalize();

struct RouteData
{
	int route[100]{};
	int count{0};
};

RouteData RouteSearch_Search(float x, float z, float target_x, float target_z);
float RouteSearch_GetXByIndex(int index);
float RouteSearch_GetZByIndex(int index);
float RouteSearch_GetYByIndex(int index); // Y座標を取得する関数を追加

int RouteSearch_PositionToIndex(float x, float z);

#endif // ROUTE_SEARCH_H
