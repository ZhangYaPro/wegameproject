#include "SkiSampleGameMode.h"
#include "SkiRunnerCharacter.h"
#include "SkiGameAudio.h"
#include "SkiJourneyAtmosphere.h"
#include "Kismet/GameplayStatics.h"
#include "AudioMixerBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "HAL/PlatformMisc.h"

bool ASkiSampleGameMode::RunFinaleChecks(float Dt)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("SkiFinaleReview")))return false;
    auto* P=Cast<ASkiRunnerCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* A=Cast<ASkiJourneyAtmosphere>(UGameplayStatics::GetActorOfClass(this,ASkiJourneyAtmosphere::StaticClass()));
    auto* Audio=Cast<ASkiGameAudio>(UGameplayStatics::GetActorOfClass(this,ASkiGameAudio::StaticClass()));
    if(!P||!A||!Audio)return true;
    static float Last=0;static int32 Day=0,Night=0;static bool StoppedDay=false,StoppedNight=false,Muted=false,DayVisible=false,NightVisible=false,Music=false;
    QASeconds+=Dt;const float T=QASeconds;
    auto At=[&](float S){return Last<S&&T>=S;};
    auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SkiQA/%s.png"),Name),true,false);};
    if(At(.5f)){FApp::SetUnfocusedVolumeMultiplier(1);UAudioMixerBlueprintLibrary::StartRecordingOutput(this,45);}
    if(At(1)){P->PreviewVictory(10);}
    if(At(4)){Shot(TEXT("FinaleDayClose"));}
    if(At(9)){Shot(TEXT("FinaleDayMenu"));}
    if(At(15)){Day=A->GetCelebrationBursts();DayVisible=A->GetVisibleSparks()>150;Music=Audio->MusicGain(1)>.4f&&Audio->MusicGain(0)<.01f&&!P->IsCelebrating();}
    if(At(16))P->ToggleAudio();
    if(At(17))Muted=Audio->MusicGain(0)==0&&Audio->MusicGain(1)==0;
    if(At(18))P->ToggleAudio();
    if(At(20))P->MenuAction(5);
    if(At(21.5f))StoppedDay=A->GetVisibleSparks()==0&&Audio->MusicGain(1)<.01f&&!P->HasFinished();
    if(At(22))P->PreviewVictory(100);
    if(At(25))Shot(TEXT("FinaleNightClose"));
    if(At(30))Shot(TEXT("FinaleNightMenu"));
    if(At(39)){Night=A->GetCelebrationBursts();NightVisible=A->GetVisibleSparks()>150;Music&=Audio->MusicGain(1)>.4f&&!P->IsCelebrating();}
    if(At(41))P->MenuAction(5);
    if(At(43))StoppedNight=A->GetVisibleSparks()==0&&Audio->MusicGain(1)<.01f&&!P->HasFinished();
    if(At(43.5f))UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("FinaleMix"),FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SkiQA")));
    if(T>45)
    {
        const bool Pass=Day>=18&&Night>=22&&DayVisible&&NightVisible&&StoppedDay&&StoppedNight&&Music&&Muted&&Audio->AssetsReady();
        auto B=[](bool V){return V?TEXT("true"):TEXT("false");};
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"day_bursts\":%d,\"night_bursts\":%d,\"day_visible\":%s,\"night_visible\":%s,\"stop_after_day_menu\":%s,\"stop_after_night_menu\":%s,\"celebration_music\":%s,\"mute\":%s}"),B(Pass),Day,Night,B(DayVisible),B(NightVisible),B(StoppedDay),B(StoppedNight),B(Music),B(Muted)),*(FPaths::ProjectSavedDir()/TEXT("SkiQA/finale_checks.json")));
        FPlatformMisc::RequestExit(false);
    }
    Last=T;return true;
}
