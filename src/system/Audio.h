// ----------------------------------------------------
// オーディオ制御 [Audio.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-07-09
// Version: 1.0
// ----------------------------------------------------


void InitAudio();
void UninitAudio();


int LoadAudio(const char* FileName);
void UnloadAudio(int Index);
void PlayAudio(int Index, bool Loop = false);
void StopAudio(int Index); // 再生中の音を停止（アンロードせず）

