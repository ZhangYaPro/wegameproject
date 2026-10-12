#include "SkiRunnerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Components/StaticMeshComponent.h"

// Opt-in state checks complement the full-route physics run. Nothing in this
// routine is called in ordinary play, and it uses the real landing/input paths.
void ASkiRunnerCharacter::RunLandingChecks()
{
    FHitResult Floor;Floor.ImpactNormal=Floor.Normal=FVector::UpVector;
    const bool Initial=Hearts==5;
    bStarted=true;Time=10;AirSeconds=1;PendingFlips=1;VisualPitch=0;
    Landed(Floor);
    const bool Safe=Hearts==5&&TotalFlips==1&&AirBonus>0&&Crashes==0;
    RestartRun();bStarted=true;
    bool CrashCostsOne=true,Protected=true,NoReward=true,InputLocked=true;
    for(int32 I=0;I<5;++I)
    {
        Time+=2;FallRemaining=0;AirSeconds=1;VisualPitch=180;PendingFlips=1;
        Landed(Floor);
        CrashCostsOne&=Hearts==4-I&&Crashes==I+1;
        NoReward&=AirBonus==0&&TotalFlips==0&&PendingFlips==0;
        if(I==0)
        {
            StartOrJump();InputLocked=IsRecovering()&&!bJumpHeld;
            AirSeconds=1;VisualPitch=180;Landed(Floor);
            Protected=Hearts==4&&Crashes==1;
        }
    }
    const bool EndsAtZero=Hearts==0&&bFinished;
    RestartRun();
    const bool Reset=Hearts==5&&!bFinished&&!IsRecovering()&&Crashes==0&&Collected==0;
    GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    GetCharacterMovement()->Velocity=FVector(900,0,1100);
    bWasAirborne=true;AirSeconds=.1f;bJumpHeld=true;TakeoffPitch=BasePitch=0;
    UpdateTricks(.1f);
    const bool NoTakeoffTilt=FMath::Abs(VisualPitch)<.01f&&SpinAngle==0;
    ReleaseJump();AdvanceAerialSpin(.6f);
    const bool TapOnlyJumps=SpinAngle==0&&PendingFlips==0&&!bReturningUpright;
    bJumpHeld=true;AirHoldSeconds=0;
    AdvanceAerialSpin(.3f);AdvanceAerialSpin(.29f);
    const bool Delay=SpinAngle==0;
    AdvanceAerialSpin(.06f);
    const bool LongHoldFlips=SpinAngle>0&&SpinVelocity<60&&FlipHoldDelay>=.6f&&FlipRate==300&&GetCharacterMovement()->GravityScale==1.6f;
    SpinAngle=180;SpinVelocity=FlipRate;bJumpHeld=true;ReleaseJump();
    AdvanceAerialSpin(.05f);
    const bool SlowForwardReturn=bReturningUpright&&SpinAngle>180&&SpinAngle<225;
    bool Monotone=true;
    float PreviousAngle=SpinAngle;
    for(int32 I=0;I<100;++I)
    {
        AdvanceAerialSpin(.02f);
        const float Unwrapped=PendingFlips*360+SpinAngle;
        Monotone&=Unwrapped+.001f>=PreviousAngle&&Unwrapped<=360.001f;
        PreviousAngle=Unwrapped;
    }
    const bool EndsUpright=SpinAngle==0&&SpinVelocity==0&&PendingFlips==1&&!bReturningUpright;
    AdvanceAerialSpin(1);
    const bool NoExtraTurn=SpinAngle==0&&PendingFlips==1;
    const float Flat=CruiseSpeedForSlope(0,false,false),Down=CruiseSpeedForSlope(-45,false,false);
    const bool GentleSlopeSpeed=Flat==1060&&Down>Flat&&Down<Flat*1.25f&&CruiseSpeedForSlope(30,false,false)<Flat&&CruiseSpeedForSlope(-61,false,true)<=1410;
    auto CheckReturn=[&](float Angle,bool Forward)
    {
        RestartRun();GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        SpinAngle=Angle;SpinVelocity=300;PendingFlips=2;bJumpHeld=true;ReleaseJump();
        float Previous=Angle;bool Direction=true;
        for(int32 I=0;I<120;++I)
        {
            AdvanceAerialSpin(.02f);
            const float Current=SpinAngle+(PendingFlips-2)*360;
            Direction&=Forward?Current>=Previous-.001f:Current<=Previous+.001f;
            Previous=Current;
        }
        return Direction&&SpinAngle==0&&!bReturningUpright&&PendingFlips==(Forward?3:2);
    };
    const bool NearestBack=CheckReturn(80,false),NearestForward=CheckReturn(280,true),HalfTurn=CheckReturn(180,true);
    RestartRun();bStarted=true;Time=10;AirSeconds=1;VisualPitch=64;PendingFlips=1;Landed(Floor);
    const bool MildSafe=Hearts==5&&TotalFlips==1&&Crashes==0;
    RestartRun();bStarted=true;Time=10;AirSeconds=1;VisualPitch=85;PendingFlips=1;Landed(Floor);
    const bool StumbleSafe=Hearts==5&&WobbleRemaining>0&&TotalFlips==0&&Crashes==0;
    RestartRun();bStarted=true;Time=10;AirSeconds=1;VisualPitch=100;Landed(Floor);
    const bool ForgivingBoundary=Hearts==5&&Crashes==0;
    RestartRun();bStarted=true;Time=10;AirSeconds=1;VisualPitch=115;Landed(Floor);
    const bool ObviousCrash=Hearts==4&&Crashes==1&&IsRecovering();
    UpdateFallAnimation(.12f);const FRotator Early=TrickPivot->GetRelativeRotation();
    UpdateFallAnimation(.40f);const FRotator FallenPose=TrickPivot->GetRelativeRotation();
    UpdateFallAnimation(.70f);const FRotator Up=TrickPivot->GetRelativeRotation();
    bool LimbsMoved=false;
    for(const FBodyPart& Part:BodyParts)if(auto* C=Part.Component.Get())LimbsMoved|=!C->GetRelativeTransform().Equals(Part.Rest,.1f);
    UpdateFallAnimation(.50f);
    const bool FallPhases=!Early.Equals(FallenPose,1)&&!FallenPose.Equals(Up,1)&&!IsRecovering()&&LimbsMoved;
    RestartRun();bStarted=true;Time=10;Hurt();
    const bool ObstaclePose=Hearts==4&&IsRecovering();
    Hearts=0;bFinished=true;UpdateFallAnimation(3);
    const bool DeathStaysDown=FMath::Abs(FMath::FindDeltaAngleDegrees(GetSlopeDegrees(),TrickPivot->GetRelativeRotation().Pitch))>60;
    UpdatePresentationCamera(1);RestartRun();
    const bool PresentationReset=DeathBlend==0&&PresentationTime==0&&!IsRecovering();
    const bool Passed=Initial&&Safe&&CrashCostsOne&&Protected&&NoReward&&InputLocked&&EndsAtZero&&Reset&&NoTakeoffTilt&&TapOnlyJumps&&Delay&&LongHoldFlips&&SlowForwardReturn&&Monotone&&EndsUpright&&NoExtraTurn&&GentleSlopeSpeed&&NearestBack&&NearestForward&&HalfTurn&&MildSafe&&StumbleSafe&&ForgivingBoundary&&ObviousCrash&&FallPhases&&ObstaclePose&&DeathStaysDown&&PresentationReset;
    IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("SkiQA")),true);
    FString Report=FString::Printf(TEXT("{\"passed\":%s,\"initial_five\":%d,\"safe_landing_reward\":%d,\"one_life_per_crash\":%d,\"repeat_hit_protected\":%d,\"no_crash_reward\":%d,\"recovery_locks_jump\":%d,\"ends_at_zero\":%d,\"restart_resets\":%d,\"no_takeoff_back_tilt\":%d,\"tap_only_jumps\":%d,\"hold_delay\":%d,\"long_hold_flips\":%d,\"slow_forward_return\":%d,\"return_monotone\":%d,\"ends_upright\":%d,\"no_extra_turn\":%d,\"gentle_slope_speed\":%d"),Passed?TEXT("true"):TEXT("false"),Initial,Safe,CrashCostsOne,Protected,NoReward,InputLocked,EndsAtZero,Reset,NoTakeoffTilt,TapOnlyJumps,Delay,LongHoldFlips,SlowForwardReturn,Monotone,EndsUpright,NoExtraTurn,GentleSlopeSpeed);
    Report+=FString::Printf(TEXT(",\"nearest_backward\":%d,\"nearest_forward\":%d,\"half_turn_tie\":%d,\"mild_landing_safe\":%d,\"stumble_no_life_loss\":%d,\"forgiving_boundary\":%d,\"obvious_crash\":%d,\"animated_fall_phases\":%d,\"obstacle_fall_pose\":%d,\"death_stays_down\":%d,\"presentation_reset\":%d}"),NearestBack,NearestForward,HalfTurn,MildSafe,StumbleSafe,ForgivingBoundary,ObviousCrash,FallPhases,ObstaclePose,DeathStaysDown,PresentationReset);
    FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("SkiQA/landing_checks.json")));
    UE_LOG(LogTemp,Display,TEXT("SKI_LANDING_CHECK %s"),*Report);
    FPlatformMisc::RequestExit(false);
}
