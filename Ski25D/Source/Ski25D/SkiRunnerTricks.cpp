#include "SkiRunnerCharacter.h"
#include "SkiGameAudio.h"
#include "SkiSampleTrail.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameViewportClient.h"
#include "Misc/Paths.h"

void ASkiRunnerCharacter::ReleaseJump()
{
    const bool WasHeld=bJumpHeld;
    if(WasHeld&&IsAirborne())bReleasedThisFlight=true;
    bJumpHeld=false;AirHoldSeconds=0;StopJumping();
    if(WasHeld&&IsAirborne())BeginUprightReturn();
}

void ASkiRunnerCharacter::BeginUprightReturn()
{
    if(bReturningUpright)return;
    if(SpinAngle<.01f){SpinAngle=SpinVelocity=0;bSpinning=false;return;}
    bReturningUpright=true;ReturnStartAngle=SpinAngle;ReturnElapsed=0;
    // Settle towards the closest upright pose. At the exact half turn, keep
    // the current direction. Undoing a partial turn must not award a flip.
    ReturnTargetAngle=SpinAngle<180.f?0.f:360.f;
    const float Distance=ReturnTargetAngle-SpinAngle;
    ReturnDuration=FMath::Max(.45f,FMath::Abs(Distance)/150.f);
    ReturnInitialSpeed=Distance*SpinVelocity>0
        ?FMath::Sign(Distance)*FMath::Min(FMath::Abs(SpinVelocity),2.5f*FMath::Abs(Distance)/ReturnDuration):0;
}

void ASkiRunnerCharacter::AdvanceAerialSpin(float Dt)
{
    const float PreviousHold=AirHoldSeconds;
    AirHoldSeconds=bJumpHeld?AirHoldSeconds+Dt:0;
    const bool Ready=bJumpHeld&&AirHoldSeconds>FlipHoldDelay;
    if(Ready&&bReturningUpright){bReturningUpright=false;SpinVelocity=FMath::Max(0.f,SpinVelocity);}
    if(bReturningUpright)
    {
        ReturnElapsed=FMath::Min(ReturnElapsed+Dt,ReturnDuration);
        const float T=ReturnElapsed/ReturnDuration,T2=T*T,T3=T2*T;
        const float Distance=ReturnTargetAngle-ReturnStartAngle;
        SpinAngle=ReturnStartAngle+(-2*T3+3*T2)*Distance+(T3-2*T2+T)*ReturnDuration*ReturnInitialSpeed;
        SpinVelocity=((-6*T2+6*T)*Distance+(3*T2-4*T+1)*ReturnDuration*ReturnInitialSpeed)/ReturnDuration;
        if(ReturnElapsed>=ReturnDuration)
        {
            SpinAngle=SpinVelocity=0;if(ReturnTargetAngle>=360)++PendingFlips;bReturningUpright=false;
        }
    }
    else if(Ready)
    {
        const float SpinDt=FMath::Max(0.f,AirHoldSeconds-FlipHoldDelay)-FMath::Max(0.f,PreviousHold-FlipHoldDelay);
        const float OldVelocity=SpinVelocity;
        const float RampTime=FMath::Min(SpinDt,FMath::Abs(FlipRate-OldVelocity)/FlipAcceleration);
        SpinVelocity=FMath::FInterpConstantTo(SpinVelocity,FlipRate,SpinDt,FlipAcceleration);
        SpinAngle+=(OldVelocity+SpinVelocity)*.5f*RampTime+SpinVelocity*(SpinDt-RampTime);
        if(SpinAngle>=360.f){PendingFlips+=FMath::FloorToInt(SpinAngle/360);SpinAngle=FMath::Fmod(SpinAngle,360.f);}
    }
    else SpinVelocity=0;
    bSpinning=FMath::Abs(SpinVelocity)>1||bReturningUpright;
}

float ASkiRunnerCharacter::PredictLanding(FVector& LandingPoint) const
{
    const FVector P=GetActorLocation(),V=GetVelocity();
    const float Gravity=GetCharacterMovement()->GetGravityZ();
    float PreviousGap=P.Z-88-ASkiSampleTrail::SurfaceHeightAt(P.X);
    for(float T=.04f;T<=4.f;T+=.04f)
    {
        const float X=P.X+V.X*T;
        const float Z=P.Z+V.Z*T+.5f*Gravity*T*T;
        const float Gap=Z-88-ASkiSampleTrail::SurfaceHeightAt(X);
        if(Gap<=0&&(PreviousGap>0||V.Z<0))
        {
            const float ImpactTime=T-.04f+.04f*FMath::Clamp(PreviousGap/FMath::Max(PreviousGap-Gap,.001f),0.f,1.f);
            LandingPoint=FVector(P.X+V.X*ImpactTime,0,ASkiSampleTrail::SurfaceHeightAt(P.X+V.X*ImpactTime));
            return ImpactTime;
        }
        PreviousGap=Gap;
    }
    LandingPoint=FVector(P.X+V.X*4,0,ASkiSampleTrail::SurfaceHeightAt(P.X+V.X*4));
    return 4;
}

void ASkiRunnerCharacter::UpdateTricks(float Dt)
{
    if(FallRemaining>0)
    {
        UpdateFallAnimation(Dt);
        return;
    }
    const bool Air=IsAirborne();
    if(Air&&!bWasAirborne)
    {
        SpinAngle=SpinVelocity=AirHoldSeconds=0;PendingFlips=0;bSpinning=false;bReturningUpright=false;bFlightDamaged=false;bReleasedThisFlight=false;
        TakeoffPitch=BasePitch;
    }
    bWasAirborne=Air;
    if(Air)
    {
        FVector Landing;
        const float Remaining=PredictLanding(Landing);
        const float LandingSlope=FMath::RadiansToDegrees(FMath::Atan2(ASkiSampleTrail::SurfaceHeightAt(Landing.X+30)-ASkiSampleTrail::SurfaceHeightAt(Landing.X-30),60));
        // A plain jump preserves the takeoff stance rather than pitching the
        // body backwards with the upward velocity. Only a sustained hold flips.
        BasePitch=FMath::FInterpTo(BasePitch,Remaining<.3f?LandingSlope:TakeoffPitch,Dt,Remaining<.3f?9.f:4.f);
        const int32 PreviousFlips=PendingFlips;
        AdvanceAerialSpin(Dt);
        if(bSpinning)
        {
            if(bAutomation&&!bNoInputTest&&!bCapturedFlip&&SpinAngle>140&&SpinAngle<220)
            {
                bCapturedFlip=true;
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SkiQA/Backflip.png"),true,false);
            }
        }
        if(PendingFlips>PreviousFlips)UE_LOG(LogTemp,Display,TEXT("SKI_FLIP_PENDING count=%d remaining=%.2f"),PendingFlips,Remaining);
        VisualPitch=BasePitch+SpinAngle;
        TrickPivot->SetRelativeLocation(FVector(0,0,-6));
    }
    else
    {
        BasePitch=FMath::FInterpTo(BasePitch,GetSlopeDegrees(),Dt,16);
        WobbleRemaining=FMath::Max(0.f,WobbleRemaining-Dt);
        const float WobbleT=1-WobbleRemaining/.65f;
        VisualPitch=BasePitch+WobbleAngle*FMath::Exp(-5*WobbleT)*FMath::Cos(WobbleT*PI*2)*(WobbleRemaining>0?1.f:0.f);
        // Keep the skis on the snow while rotating around the skier's centre.
        TrickPivot->SetRelativeLocation(FVector(0,0,-86)+FRotator(VisualPitch,0,0).RotateVector(FVector(0,0,80)));
    }
    TrickPivot->SetRelativeRotation(FRotator(VisualPitch,0,0));
    const float Tuck=bReturningUpright?.95f:bSpinning?.86f:1.f;
    const float Compression=1-.12f*FMath::Sin(FMath::Clamp(LandingPulse/.24f,0.f,1.f)*PI);
    SkierVisual->SetRelativeScale3D(FVector(1,1,FMath::FInterpTo(SkierVisual->GetRelativeScale3D().Z,Tuck*Compression,Dt,12)));
    ApplyBodyPose(bSpinning?.65f:WobbleRemaining>0?.55f:.12f,WobbleRemaining>0?.3f:0,0,FMath::Sin(Time*3)*.6f);
}

void ASkiRunnerCharacter::RefreshScore()
{
    Score=FMath::FloorToInt(DistanceScore)+AirBonus+PickupBonus;
}

void ASkiRunnerCharacter::ResetPickups()
{
    Pickups.Empty();Medals->ClearInstances();Crystals->ClearInstances();
    Medals->SetWorldTransform(FTransform::Identity);Crystals->SetWorldTransform(FTransform::Identity);
    auto AddPickup=[&](float X,float Z,bool Crystal)
    {
        const FVector Position(X,0,Z);
        // Keep neighbouring authored groups from stacking rewards together.
        for(const FPickup& Existing:Pickups)
            if(FVector::DistSquared(Existing.Position,Position)<FMath::Square(180.f))return;
        auto* Pool=Crystal?Crystals:Medals;
        const FTransform Transform(Crystal?FRotator(0,0,45):FRotator(0,0,90),Position,Crystal?FVector(.42,.42,.42):FVector(.62,.62,.10));
        const int32 Index=Pool->AddInstance(Transform,true);
        Pickups.Add({Position,Index,Crystal,false});
    };
    // Short, authored groups leave long stretches of snow and scenery clear.
    // Intro, snow village, Changbai approach, winter-fishing exit, Yanbian,
    // and the finish each have a small reward rather than a continuous ribbon.
    TArray<float> GroundGroups={1700.f,18400.f,43300.f,57900.f,67400.f,100400.f};
    if(ASkiSampleTrail::GetActiveCourse()>0){GroundGroups.Empty();for(float F:{.025f,.20f,.37f,.58f,.77f,.96f})GroundGroups.Add(F*ASkiSampleTrail::FinishX);}
    for(float Start:GroundGroups)
    {
        for(int32 I=0;I<4;++I)
        {
            const float X=Start+I*260;
            bool Clear=X<ASkiSampleTrail::FinishX-500;
            for(float Obstacle:ASkiSampleTrail::ObstaclePositions())Clear&=FMath::Abs(X-Obstacle)>550;
            if(Clear)AddPickup(X,ASkiSampleTrail::SurfaceHeightAt(X)+115,false);
        }
    }
    // Arc ribbons are authored against a reference jump, not attached to the
    // player. Different take-off times and speeds collect different portions.
    // Only selected obstacles carry a reward; others test timing on their own.
    TArray<float> RewardObstacles;const auto& Obstacles=ASkiSampleTrail::ObstaclePositions();
    for(int32 I=1;I<Obstacles.Num();I+=4)RewardObstacles.Add(Obstacles[I]);
    for(float Obstacle:RewardObstacles)
    {
        const float Start=Obstacle-650,Z0=ASkiSampleTrail::SurfaceHeightAt(Start)+88;
        for(int32 I=0;I<4;++I)
        {
            const float T=.25f+I*.20f,X=Start+1110*T;
            const float Z=Z0+1150*T-784*T*T;
            if(Z>ASkiSampleTrail::SurfaceHeightAt(X)+240)AddPickup(X,Z,I==2);
        }
    }
    TArray<float> RewardRamps;const auto& Ramps=ASkiSampleTrail::RampPositions();
    for(int32 I=0;I<Ramps.Num();I+=2)RewardRamps.Add(Ramps[I]);
    for(float Ramp:RewardRamps)
    {
        const float Z0=ASkiSampleTrail::SurfaceHeightAt(Ramp)+88;
        for(int32 I=0;I<5;++I)
        {
            const float T=.22f+I*.23f,X=Ramp+1240*T;
            const float Z=Z0+675*T-784*T*T;
            if(Z>ASkiSampleTrail::SurfaceHeightAt(X)+240)AddPickup(X,Z,I==2);
        }
    }
    UE_LOG(LogTemp,Display,TEXT("SKI_PICKUP_LAYOUT total=%d medals=%d crystals=%d"),Pickups.Num(),Medals->GetInstanceCount(),Crystals->GetInstanceCount());
}

void ASkiRunnerCharacter::UpdatePickups(float Dt)
{
    RewardTime=FMath::Max(0.f,RewardTime-Dt);
    const FVector P=GetActorLocation();
    for(FPickup& Pickup:Pickups)
    {
        if(Pickup.bTaken||FMath::Abs(Pickup.Position.X-P.X)>4500)continue;
        const FVector Position=Pickup.Position+FVector(0,0,8*FMath::Sin(Time*3+Pickup.Position.X*.005f));
        auto* Pool=Pickup.bCrystal?Crystals:Medals;
        const FRotator Rotation=Pickup.bCrystal?FRotator(0,Time*55,45):FRotator(0,Time*45,90);
        FTransform Transform(Rotation,Position,Pickup.bCrystal?FVector(.42,.42,.42):FVector(.62,.62,.10));
        const FVector Closest=FMath::ClosestPointOnSegment(Position,PreviousPickupPosition,P);
        if(FVector::DistSquared(Closest,Position)<FMath::Square(125.f))
        {
            ASkiGameAudio::Event(this,Pickup.bCrystal?TEXT("Crystal"):TEXT("Pickup"));
            Pickup.bTaken=true;Transform.SetScale3D(FVector::ZeroVector);
            ++Collected;if(IsAirborne())++AirPickups;else ++GroundPickups;
            const int32 Bonus=Pickup.bCrystal?100:25;PickupBonus+=Bonus;
            // Landed tricks retain their larger toast briefly; counters update immediately.
            if(RewardTime<.6f||Pickup.bCrystal)
            {
                RewardMessage=Pickup.bCrystal?TEXT("雾凇冰晶 +100"):TEXT("冰雪徽章 +25");RewardTime=1;
            }
            UE_LOG(LogTemp,Display,TEXT("SKI_PICKUP air=%d crystal=%d total=%d"),IsAirborne(),Pickup.bCrystal,Collected);
        }
        Pool->UpdateInstanceTransform(Pickup.Instance,Transform,true,false,true);
    }
    Medals->MarkRenderStateDirty();Crystals->MarkRenderStateDirty();
    PreviousPickupPosition=P;RefreshScore();
}
