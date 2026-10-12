#include "SkiJourneyProgress.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"

bool USkiJourneyProgress::IsTestRun()
{
    return FString(FCommandLine::Get()).Contains(TEXT("-Ski"));
}
void USkiJourneyProgress::Init()
{
    Super::Init();
    if(IsTestRun())return;
    if(auto* Save=Cast<USkiJourneySave>(UGameplayStatics::LoadGameFromSlot(TEXT("JilinJourneyHistory"),0)))
    {
        if(Save->Version!=1)return;
        if(Save->BestScores.Num()==3)for(int32 I=0;I<3;++I){ChapterScores[I]=FMath::Max(0,Save->BestScores[I]);Completed[I]=ChapterScores[I]>0;}
        for(const auto& Entry:Save->Entries)
            if(Entry.Chapter>=0&&Entry.Chapter<3&&Entry.Score>=0&&FMath::IsFinite(Entry.Seconds)&&Entry.Seconds>=0&&History.Num()<50)
            {
                History.Add(Entry);
                if(Entry.Hearts>0){ChapterScores[Entry.Chapter]=FMath::Max(ChapterScores[Entry.Chapter],Entry.Score);Completed[Entry.Chapter]=true;}
            }
    }
}
void USkiJourneyProgress::Record(int32 Chapter,int32 Score,float Seconds,int32 Flips,int32 Hearts)
{
    if(Chapter<0||Chapter>2||IsTestRun())return;
    FSkiHistoryEntry Entry;Entry.Chapter=Chapter;Entry.Score=Score;Entry.Seconds=Seconds;Entry.Flips=Flips;Entry.Hearts=Hearts;
    Entry.Date=FDateTime::Now().ToString(TEXT("%Y.%m.%d  %H:%M"));
    History.Insert(Entry,0);if(History.Num()>50)History.SetNum(50);
    if(Hearts>0){ChapterScores[Chapter]=FMath::Max(ChapterScores[Chapter],Score);Completed[Chapter]=true;}
    auto* Save=Cast<USkiJourneySave>(UGameplayStatics::CreateSaveGameObject(USkiJourneySave::StaticClass()));
    Save->Entries=History;for(int32 I=0;I<3;++I)Save->BestScores.Add(ChapterScores[I]);
    bSaveFailed=!UGameplayStatics::SaveGameToSlot(Save,TEXT("JilinJourneyHistory"),0);
    if(bSaveFailed)UE_LOG(LogTemp,Warning,TEXT("Could not save skiing history"));
}
