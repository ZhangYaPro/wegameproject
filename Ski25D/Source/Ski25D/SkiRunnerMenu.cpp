#include "SkiRunnerCharacter.h"
#include "SkiGameAudio.h"
#include "SkiSampleTrail.h"
#include "SkiJourneyProgress.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

void ASkiRunnerCharacter::MenuLeft()
{
    if(bStarted)return;
    ASkiGameAudio::Event(this,TEXT("MenuMove"));
    if(MenuPage==2)HistoryPage=FMath::Max(0,HistoryPage-1);
    else {MenuPage=1;SelectedCourse=(SelectedCourse+2)%3;}
}
void ASkiRunnerCharacter::MenuRight()
{
    if(bStarted)return;
    ASkiGameAudio::Event(this,TEXT("MenuMove"));
    if(MenuPage==2){if(auto* G=Cast<USkiJourneyProgress>(GetGameInstance()))HistoryPage=FMath::Min(FMath::Max(0,(G->History.Num()-1)/6),HistoryPage+1);}
    else {MenuPage=1;SelectedCourse=(SelectedCourse+1)%3;}
}
void ASkiRunnerCharacter::MenuUp(){if(!bStarted&&MenuPage==0){ASkiGameAudio::Event(this,TEXT("MenuMove"));MenuFocus=(MenuFocus+2)%3;}}
void ASkiRunnerCharacter::MenuDown(){if(!bStarted&&MenuPage==0){ASkiGameAudio::Event(this,TEXT("MenuMove"));MenuFocus=(MenuFocus+1)%3;}}
void ASkiRunnerCharacter::MenuConfirm()
{
    if(bStarted)return;
    ASkiGameAudio::Event(this,TEXT("MenuConfirm"));
    if(MenuPage==2){MenuPage=0;return;}
    if(MenuPage==0&&MenuFocus>0){MenuPage=MenuFocus;return;}
    if(SelectedCourse!=ASkiSampleTrail::GetActiveCourse())
    {
        if(auto* G=Cast<USkiJourneyProgress>(GetGameInstance()))G->bStartOnArrival=true;
        UGameplayStatics::OpenLevel(this,ASkiSampleTrail::CourseMap(SelectedCourse));
    }
    else {bStarted=true;PresentationTime=0;bJumpHeld=false;ShowMessage(TEXT("跃过障碍，借势滑行；沿途发现吉林风物"),4);}
}
void ASkiRunnerCharacter::MenuAction(int32 Action)
{
    if(Action==7){ToggleAudio();return;}
    if(IsCelebrating())return;
    if(Action==1||Action==2||Action==5)ASkiGameAudio::Event(this,TEXT("MenuMove"));
    if(Action==0){if(bFinished)StartOrJump();else {MenuFocus=0;MenuConfirm();}}
    else if(Action==1&&!bStarted){MenuPage=1;MenuFocus=1;}
    else if(Action==2&&!bStarted){MenuPage=2;MenuFocus=2;HistoryPage=0;}
    else if(Action==3)MenuLeft();
    else if(Action==4)MenuRight();
    else if(Action==5){if(bFinished)RestartRun();else MenuPage=MenuFocus=0;}
    else if(Action==6){RestartRun();MenuFocus=0;MenuConfirm();}
}
void ASkiRunnerCharacter::PreviewVictory(float PreviewTime)
{
    bStarted=true;Hearts=MaxHearts;Time=PreviewTime;FinishRun();
}

void ASkiRunnerCharacter::ToggleAudio()
{
    if(auto* G=Cast<USkiJourneyProgress>(GetGameInstance()))
    {
        G->bAudioMuted=!G->bAudioMuted;
        if(!G->bAudioMuted)ASkiGameAudio::Event(this,TEXT("MenuConfirm"),.6f);
    }
}
