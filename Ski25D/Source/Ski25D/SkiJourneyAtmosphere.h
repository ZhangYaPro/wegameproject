#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkiJourneyAtmosphere.generated.h"
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class UDirectionalLightComponent;
class UAudioComponent;
class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;
class ASkiSampleTrail;
class ASkiRunnerCharacter;

UCLASS()
class SKI25D_API ASkiJourneyAtmosphere : public AActor
{
    GENERATED_BODY()
public:
    ASkiJourneyAtmosphere();
    virtual void Tick(float Dt) override;
    int32 GetCelebrationBursts()const{return CelebrationBursts;}
    int32 GetVisibleSparks()const{return VisibleSparks;}
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY() UInstancedStaticMeshComponent* Snow;
    UPROPERTY() TArray<UInstancedStaticMeshComponent*> Sparks;
    UPROPERTY() TArray<UPointLightComponent*> Lamps;
    UPROPERTY() UAudioComponent* WindAudio;
    UPROPERTY() UDirectionalLightComponent* MoonFill;
    UPROPERTY() ADirectionalLight* Sun;
    UPROPERTY() ASkyLight* Sky;
    UPROPERTY() AExponentialHeightFog* Fog;
    UPROPERTY() ASkiSampleTrail* Trail;
    UPROPERTY() ASkiRunnerCharacter* Runner;
    float FXTime=0,LastRunTime=0,NextBurst=0;
    int32 BurstCount=0,CelebrationBursts=0,VisibleSparks=0;
    bool bWasCelebrating=false;
    bool bDayShot=false,bDuskShot=false,bNightShot=false,bReportWritten=false;
    struct FBurst { FVector Origin; float Start=-100; int32 Color=0; };
    FBurst Bursts[6];
};
