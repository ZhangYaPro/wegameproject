#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SkiRunnerCharacter.generated.h"
class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UInstancedStaticMeshComponent;

UCLASS()
class SKI25D_API ASkiRunnerCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    ASkiRunnerCharacter();
    virtual void Tick(float Dt) override;
    virtual void Landed(const FHitResult& Hit) override;
    void StartOrJump();
    void SelectRime();
    void SelectBeidahu();
    void SelectChangbai();
    void PreviewCoursePoint(float Fraction);
    void RestartRun();
    void TogglePause();
    void ToggleAudio();
    int32 MenuPage=0,MenuFocus=0,SelectedCourse=0,HistoryPage=0;
    void MenuLeft();
    void MenuRight();
    void MenuUp();
    void MenuDown();
    void MenuConfirm();
    void MenuAction(int32 Action);
    bool IsCelebrating()const{return bFinished&&Hearts>0&&PresentationTime<5.5f;}
    float GetPresentationSeconds()const{return PresentationTime;}
    void PreviewVictory(float PreviewTime=100);
    bool HasStarted() const { return bStarted; }
    bool HasFinished() const { return bFinished; }
    bool IsRunPaused() const { return bRunPaused; }
    int32 GetHearts() const { return Hearts; }
    static constexpr int32 MaxHearts=5;
    bool IsRecovering() const { return FallRemaining>0; }
    float GetDeathBlend() const { return DeathBlend; }
    int32 GetScore() const { return Score; }
    float GetProgress() const;
    int32 GetLandings() const { return Landings; }
    FString GetRegion() const;
    FString GetMessage() const { return Message; }
    float GetMessageTime() const { return MessageTime; }
    float GetSpeedKmh() const { return GetVelocity().Size()*.036f; }
    float GetDropMetres() const;
    float GetSlopeDegrees() const;
    bool IsBraking() const { return bBraking; }
    bool IsAirborne() const;
    float GetRunSeconds() const { return Time; }
    int32 GetCollected() const { return Collected; }
    int32 GetTotalFlips() const { return TotalFlips; }
    int32 GetPendingFlips() const { return PendingFlips; }
    bool IsFlipping() const { return bSpinning; }
    bool IsReturningUpright() const { return bReturningUpright; }
    FString GetRewardMessage() const { return RewardMessage; }
    float GetRewardTime() const { return RewardTime; }
protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
private:
    UPROPERTY(VisibleAnywhere) USpringArmComponent* CameraBoom;
    UPROPERTY(VisibleAnywhere) UCameraComponent* FollowCamera;
    UPROPERTY() class UPointLightComponent* VictoryLight;
    UPROPERTY(VisibleAnywhere) USceneComponent* SkierVisual;
    UPROPERTY(VisibleAnywhere) USceneComponent* TrickPivot;
    UPROPERTY() UInstancedStaticMeshComponent* Medals;
    UPROPERTY() UInstancedStaticMeshComponent* Crystals;
    UPROPERTY() UInstancedStaticMeshComponent* SkiSpray;
    UStaticMeshComponent* Shape(const TCHAR* Name,const TCHAR* MeshName,FVector P,FVector S,UMaterialInterface* Mat,FRotator R=FRotator::ZeroRotator);
    void Hurt();
    void BrakePressed();
    void BrakeReleased();
    void SelectCourse(int32 Index);
    static float CruiseSpeedForSlope(float Slope,bool Braking,bool Boosted);
    void UpdateCamera(float Dt);
    void ReleaseJump();
    void UpdateTricks(float Dt);
    void AdvanceAerialSpin(float Dt);
    void BeginUprightReturn();
    void BeginFallAnimation();
    void UpdateFallAnimation(float Dt);
    void CacheBodyPose();
    void ApplyBodyPose(float Crouch,float Flail,float Reach,float Breath=0);
    void UpdateVictoryPose();
    void PoseArms(float Crouch,float Flail,float Reach,float Victory=0);
    bool UpdatePresentationCamera(float Dt);
    void UpdatePresentationCheck(float Dt);
    void UpdateSkiSpray(float Dt);
    void RunLandingChecks();
    float PredictLanding(FVector& LandingPoint) const;
    void ResetPickups();
    void UpdatePickups(float Dt);
    void RefreshScore();
    void FinishRun();
    void ShowMessage(const FString& Text,float Seconds=3);
    bool bStarted=false,bFinished=false,bRunPaused=false,bAutomation=false,bNoInputTest=false,bBraking=false;
    int32 Hearts=MaxHearts,Score=0,Landings=0,LastRegion=-1;
    float Time=0,HurtUntil=0,DistanceScore=0,MessageTime=0,LastX=0,LastFeet=0,NextScreenshot=0;
    FString Message;
    TSet<int32> PassedObstacles;
    TSet<int32> UsedRamps;
    float ForwardSpeed=760,AirSeconds=0,LandingPulse=0;
    float TopSpeed=0,MaxAirHeight=0,MinArm=10000,MaxArm=0,MaxSlope=0;
    int32 AirBonus=0,RampLaunches=0;
    bool bBrakeVerified=false;
    struct FPickup { FVector Position; int32 Instance; bool bCrystal; bool bTaken=false; };
    TArray<FPickup> Pickups;
    FVector PreviousPickupPosition=FVector::ZeroVector;
    bool bJumpHeld=false,bSpinning=false,bFlightDamaged=false,bWasAirborne=false;
    float SpinAngle=0,BasePitch=0,VisualPitch=0,BoostUntil=0,RewardTime=0;
    int32 PendingFlips=0,TotalFlips=0,BestCombo=0,Collected=0,GroundPickups=0,AirPickups=0,PickupBonus=0;
    int32 ReleasedFlights=0,HoldFlights=0;
    bool bReleasedThisFlight=false;
    bool bCapturedFlip=false;
    FString RewardMessage;
    float LastLandingTime=-100;
    int32 LastLandingBonus=0,LastLandingFlips=0;
    bool bLastLandingReleased=false;
    TArray<int32> LandingFlipHistory;
    float SpinVelocity=0,FallRemaining=0,SprayAccumulator=0;
    static constexpr float FlipHoldDelay=.60f;
    static constexpr float FlipRate=300.f;
    static constexpr float FlipAcceleration=600.f;
    float AirHoldSeconds=0,TakeoffPitch=0;
    bool bReturningUpright=false;
    float ReturnStartAngle=0,ReturnTargetAngle=0,ReturnElapsed=0,ReturnDuration=0,ReturnInitialSpeed=0;
    static constexpr float FallDuration=1.65f;
    float FallStartPitch=0,FallDirection=1,WobbleRemaining=0,WobbleAngle=0;
    struct FBodyPart { TWeakObjectPtr<UStaticMeshComponent> Component; FTransform Rest; };
    TArray<FBodyPart> BodyParts;
    float PresentationTime=0,DeathBlend=0;
    bool bPresentationTest=false;
    float PresentationQASeconds=0,DeathQASeconds=0;
    float QAYawMin=1000,QAYawMax=-1000,QADeathArmMin=10000,QADeathArmMax=0;
    int32 Crashes=0,NextSpray=0,SprayEmitted=0;
    bool bCrashTest=false,bCapturedFall=false,bCapturedSpray=false;
    int32 FallCaptureStage=0;
    struct FSkiSnow { FVector Position=FVector::ZeroVector,Velocity=FVector::ZeroVector; float Age=0,Life=0,Size=0; };
    TArray<FSkiSnow> SkiSnow;
    FRandomStream SprayRandom{7321};
};
