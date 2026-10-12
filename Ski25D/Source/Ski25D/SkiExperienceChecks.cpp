#include "SkiSampleGameMode.h"
#include "SkiRunnerCharacter.h"
#include "SkiJourneyProgress.h"
#include "SkiGameAudio.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"

bool ASkiSampleGameMode::RunExperienceChecks(float Dt)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("SkiExperienceReview")))return false;
    auto* PC=GetWorld()->GetFirstPlayerController();auto* P=PC?Cast<ASkiRunnerCharacter>(PC->GetPawn()):nullptr;
    auto* HUD=PC?Cast<ASkiSampleHUD>(PC->GetHUD()):nullptr;
    auto* Audio=Cast<ASkiGameAudio>(UGameplayStatics::GetActorOfClass(this,ASkiGameAudio::StaticClass()));
    if(!P||!HUD||!Audio)return true;
    static bool Settled[3]={false,false,false},Transition=false,Muted=false,MutedEvent=false,Fast=false;
    static float Previous=0;
    QASeconds+=Dt;const float T=QASeconds;
    auto At=[&](float When){return Previous<When&&T>=When;};
    auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SkiQA/%s.png"),Name),true,false);};
    if(At(.1f)){FApp::SetUnfocusedVolumeMultiplier(1);P->MenuPage=1;P->SelectedCourse=0;}
    if(At(.5f))UAudioMixerBlueprintLibrary::StartRecordingOutput(this,26);
    if(At(1.5f)){Settled[0]=HUD->SceneWeight(0)>.98f&&Audio->MusicGain(0)>.35f;Shot(TEXT("SceneBackground1"));}
    if(At(2.f))P->MenuRight();
    if(At(2.25f)){Transition=HUD->SceneWeight(1)>.05f&&HUD->SceneWeight(1)<.97f;Shot(TEXT("SceneTransitionBlend"));}
    if(At(3.4f)){Settled[1]=HUD->SceneWeight(1)>.98f&&Audio->MusicGain(0)>.35f;Shot(TEXT("SceneBackground2"));}
    if(At(4.f))P->MenuRight();
    if(At(5.4f)){Settled[2]=HUD->SceneWeight(2)>.98f&&Audio->MusicGain(0)>.35f;Shot(TEXT("SceneBackground3"));}
    if(At(6.f)||At(6.15f)||At(6.3f)||At(6.45f))P->MenuRight();
    if(At(7.7f))Fast=P->SelectedCourse==0&&HUD->SceneWeight(0)>.98f;
    if(At(8.f))P->ToggleAudio();
    if(At(8.35f))
    {
        Muted=Audio->MusicGain(0)==0&&Audio->MusicGain(1)==0&&Audio->MusicGain(2)==0&&Audio->GlideGain()==0;
        const int32 Count=Audio->EventCount(TEXT("Pickup"));ASkiGameAudio::Event(this,TEXT("Pickup"));MutedEvent=Count==Audio->EventCount(TEXT("Pickup"));
    }
    if(At(8.8f))P->ToggleAudio();
    if(At(10.f))P->MenuConfirm();
    if(At(11.2f))P->StartOrJump();
    if(At(12.f))Shot(TEXT("WelcomeRoad"));
    if(At(14.8f))Shot(TEXT("AudioGameplay"));
    if(At(17.f)){P->RestartRun();ASkiGameAudio::Event(this,TEXT("Crystal"));}
    if(At(18.f))ASkiGameAudio::Event(this,TEXT("Trick"));
    if(At(18.4f))ASkiGameAudio::Event(this,TEXT("Land"));
    if(At(18.8f))ASkiGameAudio::Event(this,TEXT("Pickup"));
    if(At(19.f))ASkiGameAudio::Event(this,TEXT("Crash"));
    if(At(20.f))ASkiGameAudio::Event(this,TEXT("Defeat"));
    if(At(22.f))ASkiGameAudio::Event(this,TEXT("Victory"));
    if(At(24.5f))UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("ExperienceMix"),FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SkiQA")));
    if(T>26)
    {
        const bool Pass=Settled[0]&&Settled[1]&&Settled[2]&&Transition&&Muted&&MutedEvent&&Fast&&Audio->AssetsReady()&&Audio->EventCount(TEXT("Jump"))>0;
        auto B=[](bool V){return V?TEXT("true"):TEXT("false");};
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"scene_1\":%s,\"scene_2\":%s,\"scene_3\":%s,\"transition\":%s,\"rapid_switch\":%s,\"mute_loops\":%s,\"mute_events\":%s,\"audio_assets\":%s,\"jump_events\":%d,\"pickup_events\":%d,\"land_events\":%d}"),B(Pass),B(Settled[0]),B(Settled[1]),B(Settled[2]),B(Transition),B(Fast),B(Muted),B(MutedEvent),B(Audio->AssetsReady()),Audio->EventCount(TEXT("Jump")),Audio->EventCount(TEXT("Pickup")),Audio->EventCount(TEXT("Land"))),*(FPaths::ProjectSavedDir()/TEXT("SkiQA/experience_checks.json")));
        FPlatformMisc::RequestExit(false);
    }
    Previous=T;return true;
}
