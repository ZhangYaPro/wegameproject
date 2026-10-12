#include "SkiGameAudio.h"
#include "SkiRunnerCharacter.h"
#include "SkiJourneyProgress.h"
#include "SkiSampleTrail.h"
#include "Components/AudioComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "UObject/ConstructorHelpers.h"

ASkiGameAudio::ASkiGameAudio()
{
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.bTickEvenWhenPaused=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    const TCHAR* Tracks[]={TEXT("MusicWinter"),TEXT("MusicCelebration")};
    for(const TCHAR* Name:Tracks)
    {
        auto* C=CreateDefaultSubobject<UAudioComponent>(Name);C->SetupAttachment(RootComponent);
        C->bAutoActivate=false;C->bAllowSpatialization=false;C->bIsUISound=true;
        ConstructorHelpers::FObjectFinder<USoundWave> Wave(*FString::Printf(TEXT("/Game/Ski25D/Audio/%s.%s"),Name,Name));
        if(Wave.Succeeded())C->SetSound(Wave.Object);Music.Add(C);
    }
    Snow=CreateDefaultSubobject<UAudioComponent>(TEXT("SnowGlide"));Wood=CreateDefaultSubobject<UAudioComponent>(TEXT("WoodGlide"));
    for(auto* C:{Snow,Wood})
    {
        C->SetupAttachment(RootComponent);C->bAutoActivate=false;C->bAllowSpatialization=false;
        const FString Name=C->GetName();
        ConstructorHelpers::FObjectFinder<USoundWave> Wave(*FString::Printf(TEXT("/Game/Ski25D/Audio/%s.%s"),*Name,*Name));
        if(Wave.Succeeded())C->SetSound(Wave.Object);
    }
    for(const TCHAR* Name:{TEXT("MenuMove"),TEXT("MenuConfirm"),TEXT("Pickup"),TEXT("Crystal"),TEXT("Jump"),TEXT("Land"),TEXT("Crash"),TEXT("Trick"),TEXT("Victory"),TEXT("Defeat"),TEXT("Firework")})
    {
        ConstructorHelpers::FObjectFinder<USoundWave> Wave(*FString::Printf(TEXT("/Game/Ski25D/Audio/%s.%s"),Name,Name));
        if(Wave.Succeeded())Sounds.Add(FName(Name),Wave.Object);
    }
}
bool ASkiGameAudio::AssetsReady()const
{
    return Music.Num()==2&&Music[0]->Sound&&Music[1]->Sound&&Music[0]->Sound->GetDuration()>240&&Music[1]->Sound->GetDuration()>60&&Snow->Sound&&Wood->Sound&&Sounds.Num()==11;
}
void ASkiGameAudio::Event(const UObject* WorldContext,FName Name,float Volume)
{
    if(auto* A=Cast<ASkiGameAudio>(UGameplayStatics::GetActorOfClass(WorldContext,StaticClass())))A->Play(Name,Volume);
}
void ASkiGameAudio::Play(FName Name,float Volume)
{
    auto* G=Cast<USkiJourneyProgress>(GetGameInstance());if(G&&G->bAudioMuted)return;
    auto** Sound=Sounds.Find(Name);if(!Sound||!*Sound)return;
    const double Now=GetWorld()->GetRealTimeSeconds();
    const double Cooldown=Name==TEXT("Firework")?.60:Name==TEXT("Land")?.18:.055;
    if(const double* Previous=LastEvent.Find(Name))if(Now-*Previous<Cooldown)return;
    ActiveEffects.RemoveAll([](const auto& C){return !C.IsValid()||!C->IsPlaying();});
    if(ActiveEffects.Num()>=10){if(ActiveEffects[0].IsValid())ActiveEffects[0]->Stop();ActiveEffects.RemoveAt(0);}
    const float Pitch=(Name==TEXT("Pickup")||Name==TEXT("Land"))?FMath::FRandRange(.95f,1.05f):1.f;
    auto* Effect=UGameplayStatics::SpawnSound2D(this,*Sound,FMath::Clamp(Volume,0.f,1.f)*.68f,Pitch);
    if(Effect){Effect->bIsUISound=Name==TEXT("MenuMove")||Name==TEXT("MenuConfirm");ActiveEffects.Add(Effect);}
    LastEvent.Add(Name,Now);++Counts.FindOrAdd(Name);
    UE_LOG(LogTemp,Verbose,TEXT("SKI_AUDIO_EVENT %s"),*Name.ToString());
}
void ASkiGameAudio::Tick(float Dt)
{
    Super::Tick(Dt);
    if(!Runner)Runner=Cast<ASkiRunnerCharacter>(UGameplayStatics::GetPlayerPawn(this,0));if(!Runner)return;
    const auto* G=Cast<USkiJourneyProgress>(GetGameInstance());const bool Muted=G&&G->bAudioMuted;
    const bool Celebrating=Runner->HasFinished()&&Runner->GetHearts()>0;
    const int32 Selected=Celebrating?1:0;
    const float Level=Muted?0.f:Celebrating?.46f:Runner->HasFinished()?.15f:Runner->IsRunPaused()?.18f:Runner->HasStarted()?.34f:.42f;
    auto Loop=[&](UAudioComponent* C,float& Gain,float Target,float Pitch,float Rate)
    {
        Gain=Muted?0.f:FMath::FInterpConstantTo(Gain,Target,FMath::Min(Dt,.1f),Rate);
        C->SetVolumeMultiplier(Gain);C->SetPitchMultiplier(Pitch);
        if(Gain>.001f&&!C->IsPlaying())C->Play();else if(Gain<=.001f&&C->IsPlaying())C->Stop();
    };
    for(int32 I=0;I<Music.Num();++I)Loop(Music[I],Gains[I],I==Selected?Level:0,1,.5f);
    const auto* Movement=Runner->GetCharacterMovement();const auto* Floor=Movement->CurrentFloor.HitResult.GetComponent();
    const bool OnWood=Floor&&Floor->GetCollisionObjectType()==ECC_GameTraceChannel1;
    const bool Sliding=Runner->HasStarted()&&!Runner->HasFinished()&&!Runner->IsRunPaused()&&Movement->IsMovingOnGround();
    const float Speed=FMath::Clamp(Runner->GetVelocity().Size()/1250.f,0.f,1.f);
    const float Scrape=Sliding?Speed*(Runner->IsBraking()?.48f:Runner->IsRecovering()?.40f:.25f):0;
    Loop(Snow,SnowGain,OnWood?0:Scrape,.85f+Speed*.25f,2.8f);
    Loop(Wood,WoodGain,OnWood?Scrape*.85f:0,.9f+Speed*.2f,3.5f);
    if(Muted)for(auto& C:ActiveEffects)if(C.IsValid())C->Stop();
}
