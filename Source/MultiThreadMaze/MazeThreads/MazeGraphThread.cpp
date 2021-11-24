#include "MazeGraphThread.h"
#include <random>


void FMazeGraphThread::DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	GenerateMazeSegment(CurrentThread);
}

void FMazeGraphThread::GenerateMazeSegment(ENamedThreads::Type CurrentThread)
{
	WallsInfo.Empty();
	ResultWallsInfo.Empty();

	// ��������� ������ ���� � ID
	// �������������� ���� �����-�������, ������-����
	int32 CellID = 0;
	for (int32 y = 0; y < SegmentSideSize; y++)
	{
		bool bIsLastRow = y == (SegmentSideSize - 1);
		for (int32 x = 0; x < SegmentSideSize; x++)
		{
			if (x != (SegmentSideSize - 1))  // �� ������� �������� ������ ������
			{
				//������ ������: ������� ID � ��������� ID
				WallsInfo.Add(FWallInfo(CellID, CellID + 1, x, y, false));
			}
			if (!bIsLastRow) // �� ������� ������� ������ ������
			{
				//������ ������: ������� ID � ID ������ ����
				WallsInfo.Add(FWallInfo(CellID, CellID + SegmentSideSize, x, y, true));
			}
			CellID++;
		}
	}

	// ���� ������ ���� ���� ��������� ���� � ������� ����
	while (IsHasDifferentZone())
	{
		int32 WallNum = GetRandomInt(0, WallsInfo.Num() - 1);  // �������� ��������� ����� �� �������
		if (WallsInfo[WallNum].OneSideID != WallsInfo[WallNum].OtherSideID) 
		{
			// ���� � ����� ID ������, �� ������ OtherSideID �� OneSideID � ���� ���� � ID = OtherSideID � ������� ����� 
			ChangeZone(WallsInfo[WallNum].OtherSideID, WallsInfo[WallNum].OneSideID);
			auto a = WallsInfo[WallNum];
			WallsInfo.RemoveAt(WallNum);
		}
		else
		{
			// ���� ID � ����� ����������, �� ��������� ����� � �������������� ������ � ������� �� ������� ����
			auto a = WallsInfo[WallNum];
			if (bIsShow)
			{
				// ���� �������� � ����-������, �� ������ �������, ��� ���� �������� �����
				ShowWall(CurrentThread, a);
			}
			ResultWallsInfo.Add(WallsInfo[WallNum]);
			WallsInfo.RemoveAt(WallNum);
		}
	}
	// ���� � ����-������, �� ��������� ���������� � ������� �����
	// ���� � ���������� ������, �� ��������� ���������� ����� � ������������� ������
	if (bIsShow)
	{
		for (auto Wall : WallsInfo)
		{
			ShowWall(CurrentThread, Wall);
		}
	}
	else
	{
		ResultWallsInfo.Append(WallsInfo);
	}

	// ���� ���� ������� ������� ��������� ����� �������� �� �������
	if (SidesWallsFlag > 0)
	{
		// �������� ��������� ������� � ������ � ������ ��������
		const auto HoleX = GetRandomInt(0, SegmentSideSize - 1);
		const auto HoleY = GetRandomInt(0, SegmentSideSize - 1);
		for (int32 a = 0; a < SegmentSideSize; a++)
		{
			// �������/���������� ������ ��������� ����� � ��������
			if (((SidesWallsFlag % 2) > 0) && (a != HoleX))
			{
				if (bIsShow)
				{
					ShowWall(CurrentThread, FWallInfo(0, 0, SegmentSideSize - 1, a, false));
				}
				else
				{
					ResultWallsInfo.Add(FWallInfo(0, 0, SegmentSideSize - 1, a, false));
				}
			}
			// �������/���������� ������ ��������� ����� � ��������
			if ((SidesWallsFlag > 1) && (a != HoleY))
			{
				if (bIsShow)
				{
					ShowWall(CurrentThread, FWallInfo(0, 0, a, SegmentSideSize - 1, true));
				}
				else
				{
					ResultWallsInfo.Add(FWallInfo(0, 0, a, SegmentSideSize - 1, true));
				}
			}
		}
	}

	// �������� ������ ���� ��� ��������
	SegmentResult.WallsInfo.Empty();

	// ���� �� ����-�����, �� ��������� ����� �� ��������������� ������� � ��������� ��� ��������
	if (!bIsShow)
	{
		SegmentResult.WallsInfo.Append(ResultWallsInfo);
	}

	// �������� �� ��������� ��������� ��������
	TGraphTask<FTask_FinishMazeBlock>::CreateTask(NULL, CurrentThread).ConstructAndDispatchWhenReady(OnMazeSegmentWorkDone, SegmentResult);
}

// �������� �� ������� ���� � ������� ID 
bool FMazeGraphThread::IsHasDifferentZone()
{
	for (auto aaaa : WallsInfo)
	{
		if (aaaa.OneSideID != aaaa.OtherSideID) return true;
	}
	return false;
}

// ������ ID � ���� 
void FMazeGraphThread::ChangeZone(int32 OldZone, int32 NewZone)
{
	for (int32 a = 0; a < WallsInfo.Num(); a++)
	{
		if (WallsInfo[a].OneSideID == OldZone) WallsInfo[a].OneSideID = NewZone;
		if (WallsInfo[a].OtherSideID == OldZone) WallsInfo[a].OtherSideID = NewZone;
	}
	for (int32 a = 0; a < ResultWallsInfo.Num(); a++)
	{
		if (ResultWallsInfo[a].OneSideID == OldZone) ResultWallsInfo[a].OneSideID = NewZone;
		if (ResultWallsInfo[a].OtherSideID == OldZone) ResultWallsInfo[a].OtherSideID = NewZone;
	}
}

// �������� � ������������� �������� ����� � ����-������ � ����� ����� � ������
void FMazeGraphThread::ShowWall(ENamedThreads::Type CurrentThread, FWallInfo Wall)
{
	FMazeWallDrawInfo Res = FMazeWallDrawInfo(Wall.WallX, Wall.WallY, Wall.bIsVertical, SegmentResult.StartLocation, SegmentResult.BlockColor);
	TGraphTask<FTask_MazeWallShow>::CreateTask(NULL, CurrentThread).ConstructAndDispatchWhenReady(MazeWallShow, Res);
	FPlatformProcess::Sleep(0.5f);
}

int FMazeGraphThread::GetRandomInt(int32 min, int32 max)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distr(min, max);
	return distr(gen);
}