#include "SkiRunnerCharacter.h"
#include "SkiSampleTrail.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/GameViewportClient.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"

void ASkiRunnerCharacter::CacheBodyPose()
{
    BodyParts.Empty();
    TArray<USceneComponent*> PoseChildren;SkierVisual->GetChildrenComponents(false,PoseChildren);
    for(USceneComponent* Child:PoseChildren)
        if(auto* Part=Cast<UStaticMeshComponent>(Child))BodyParts.Add({Part,Part->GetRelativeTransform()});
}

void ASkiRunnerCharacter::ApplyBodyPose(float Crouch,float Flail,float Reach,float Breath)
{
    // Animate existing individual pieces about their authored rest poses.
    // No cumulative transforms, stretching limbs, or ragdoll collision needed.
    for(const FBodyPart& Part:BodyParts)
    {
        auto* C=Part.Component.Get();if(!C)continue;
        const FString Name=C->GetName();
        const float Side=Name.EndsWith(TEXT("-1"))?-1.f:1.f;
        FVector Offset=FVector::ZeroVector;FRotator Turn=FRotator::ZeroRotator;
        if(Name.StartsWith(TEXT("Thigh"))){Turn.Pitch=-24*Crouch+Side*12*Flail;Offset=FVector(-5*Crouch,Side*5*Flail,-10*Crouch);}
        else if(Name.StartsWith(TEXT("Shin"))){Turn.Pitch=18*Crouch-Side*14*Flail;Offset=FVector(-3*Crouch,Side*4*Flail,-3*Crouch);}
        else if(Name.StartsWith(TEXT("Sleeve"))){Turn=FRotator(-28*Crouch-55*Reach,Side*(25*Flail),Side*45*Flail);Offset=FVector(10*Reach,Side*13*Flail,-15*Crouch);}
        else if(Name.StartsWith(TEXT("Glove"))){Offset=FVector(22*Reach-8*Crouch,Side*24*Flail,18*Flail-22*Crouch-12*Reach);}
        else if(Name.StartsWith(TEXT("Pole"))){Turn=FRotator(-25*Crouch-35*Reach,Side*22*Flail,Side*40*Flail);Offset=FVector(16*Reach,Side*22*Flail,-12*Crouch);}
        else if(Name.StartsWith(TEXT("Ski"))){Turn=FRotator(Side*9*Flail,Side*12*Flail,0);}
        else if(Name==TEXT("Jacket")||Name==TEXT("Face")||Name==TEXT("ChestBand")||Name.StartsWith(TEXT("LeftChest"))||Name==TEXT("Zip")||Name==TEXT("Vest")||Name==TEXT("Hood")||Name==TEXT("Helmet")||Name==TEXT("Goggles")||Name==TEXT("Chin")||Name==TEXT("Scarf"))
            Offset=FVector(9*Crouch,0,-21*Crouch+Breath*1.5f);
        C->SetRelativeLocation(Part.Rest.GetLocation()+Offset);
        C->SetRelativeRotation(Part.Rest.GetRotation().Rotator()+Turn);
        C->SetRelativeScale3D(Part.Rest.GetScale3D());
    }
    PoseArms(Crouch,Flail,Reach);
}

void ASkiRunnerCharacter::PoseArms(float Crouch,float Flail,float Reach,float Victory)
{
    const FVector Body(9*Crouch,0,-21*Crouch);
    for(int32 Side:{-1,1})
    {
        const float Wave=FMath::Sin(PresentationTime*6.5f+Side*.3f)*2.0f*Victory;
        const FVector Shoulder=FVector(1,Side*22,128)+Body;
        const FVector Elbow=FMath::Lerp(FVector(7+Reach*15,Side*(29+Flail*19),101+Flail*18)+Body,FVector(2,Side*43,150+Wave),Victory);
        const FVector Hand=FMath::Lerp(FVector(32+Reach*19,Side*(31+Flail*34),83+Flail*27-12*Crouch),FVector(5,Side*60,178+Wave),Victory);
        for(const FBodyPart& Part:BodyParts)
        {
            auto* C=Part.Component.Get();if(!C)continue;const FString Name=C->GetName();
            if(Name==FString::Printf(TEXT("Sleeve%d"),Side)||Name==FString::Printf(TEXT("Forearm%d"),Side))
            {
                const bool Upper=Name.StartsWith(TEXT("Sleeve"));const FVector A=Upper?Shoulder:Elbow,B=Upper?Elbow:Hand;
                C->SetRelativeLocation((A+B)*.5);C->SetRelativeRotation(FRotationMatrix::MakeFromZ(B-A).Rotator());
                FVector S=Part.Rest.GetScale3D();S.Z=(B-A).Size()/100+.055;C->SetRelativeScale3D(S);
            }
            else if(Name==FString::Printf(TEXT("Glove%d"),Side)){C->SetRelativeLocation(Hand);C->SetRelativeRotation(FRotationMatrix::MakeFromZ(Hand-Elbow).Rotator());}
            else if(Name==FString::Printf(TEXT("Pole%d"),Side))
            {
                const FVector End=Hand+FMath::Lerp(FVector(-43,0,-83),FVector(0,0,-102),Victory);
                C->SetRelativeLocation((Hand+End)*.5);C->SetRelativeRotation(FRotationMatrix::MakeFromZ(Hand-End).Rotator());
                FVector S=Part.Rest.GetScale3D();S.Z=(Hand-End).Size()/100;C->SetRelativeScale3D(S);
            }
            else if(Side==1&&(Name==TEXT("RightArmFlag")||Name==TEXT("RightArmStars")||Name==TEXT("RightArmBorder")))
            {
                const FVector Up=(Shoulder-Elbow).GetSafeNormal();
                // Place the patch on the outer-front shoulder so it remains readable
                // in both the side-on skiing view and the raised-arm close-up.
                const FVector Facing(.85f,1,0);
                const FVector Outside=(Facing-Up*FVector::DotProduct(Facing,Up)).GetSafeNormal();
                // Seen from outside, the large star must remain at upper left.
                const FVector Right=FVector::CrossProduct(Outside,Up).GetSafeNormal();
                C->SetRelativeLocation(FMath::Lerp(Shoulder,Elbow,.45f)+Outside*(Name.EndsWith(TEXT("Stars"))?14.3f:Name.EndsWith(TEXT("Border"))?14.f:14.15f));
                C->SetRelativeRotation(FRotationMatrix::MakeFromXZ(Right,Up).Rotator());
            }
        }
    }
}

void ASkiRunnerCharacter::BeginFallAnimation()
{
    ++Crashes;FallRemaining=FallDuration;
    FallStartPitch=FMath::FindDeltaAngleDegrees(GetSlopeDegrees(),VisualPitch);
    FallDirection=FallStartPitch>=0?1.f:-1.f;
    // An obstacle strike tips forwards; an inverted landing follows its lean.
    if(FMath::Abs(FallStartPitch)<30)FallDirection=-1;
    bJumpHeld=bSpinning=bReturningUpright=false;WobbleRemaining=0;
    SpinAngle=SpinVelocity=AirHoldSeconds=0;
}

void ASkiRunnerCharacter::UpdateFallAnimation(float Dt)
{
    const bool Dead=bFinished&&Hearts<=0;
    FallRemaining=FMath::Max(0.f,FallRemaining-Dt);
    const float Elapsed=FallDuration-FallRemaining;
    const float Impact=FMath::SmoothStep(0.f,.30f,Elapsed);
    const float Settle=FMath::SmoothStep(.30f,.70f,Elapsed);
    const float Stand=Dead?0.f:FMath::SmoothStep(.92f,FallDuration,Elapsed);
    const float FallenAngle=FallDirection*78;
    const float PitchOffset=FMath::Lerp(FMath::Lerp(FallStartPitch,FallenAngle,Impact),0.f,Stand);
    const float Roll=FallDirection*FMath::Sin(Impact*PI*.5f)*22*(1-Stand);
    const float Flail=(1-Settle)*FMath::Sin(Impact*PI)*.8f;
    const float Reach=FMath::Sin(Stand*PI);
    ApplyBodyPose(FMath::Lerp(.75f,.12f,Stand),Flail,Reach);
    SkierVisual->SetRelativeScale3D(FVector::OneVector);
    const FRotator Pose(GetSlopeDegrees()+PitchOffset,0,Roll);
    TrickPivot->SetRelativeRotation(Pose);
    // Place the lowest posed component just above the local slope. This keeps
    // the helmet/arms from sinking through snow during the impact and get-up.
    float Lowest=0;bool First=true;
    const FVector Origin=GetActorLocation();
    for(const FBodyPart& Part:BodyParts)
    {
        auto* C=Part.Component.Get();if(!C||!C->GetStaticMesh())continue;
        if(C->GetName().StartsWith(TEXT("Ski"))||C->GetName().StartsWith(TEXT("Pole")))continue;
        const FBox Bounds=C->GetStaticMesh()->GetBoundingBox();
        for(int32 Corner=0;Corner<8;++Corner)
        {
            const FVector Local(Corner&1?Bounds.Max.X:Bounds.Min.X,Corner&2?Bounds.Max.Y:Bounds.Min.Y,Corner&4?Bounds.Max.Z:Bounds.Min.Z);
            const FVector Point=Pose.RotateVector(SkierVisual->GetRelativeTransform().TransformPosition(C->GetRelativeTransform().TransformPosition(Local)));
            const float Gap=Origin.Z+Point.Z-ASkiSampleTrail::HeightAt(Origin.X+Point.X);
            Lowest=First?Gap:FMath::Min(Lowest,Gap);First=false;
        }
    }
    const float GroundedZ=First?-6.f:4-Lowest;
    const float NormalZ=-86+Pose.RotateVector(FVector(0,0,80)).Z;
    const FVector PivotLocation(0,0,FMath::Lerp(GroundedZ,NormalZ,Stand));
    TrickPivot->SetRelativeLocation(PivotLocation);
    // Skis splay along the snow on impact, then blend back to the boots.
    // Equipment must not prop a horizontal body in mid-air.
    for(const FBodyPart& Part:BodyParts)
    {
        auto* C=Part.Component.Get();if(!C)continue;
        const FString Name=C->GetName();const bool Ski=Name.StartsWith(TEXT("Ski")),Pole=Name.StartsWith(TEXT("Pole"));
        if(!Ski&&!Pole)continue;
        const float Side=Name.EndsWith(TEXT("-1"))?-1.f:1.f;
        const FVector Rest=Part.Rest.GetLocation();
        const float X=Origin.X-95+Rest.X;
        const FVector SnowPoint(X,Origin.Y+Side*(Ski?28:52),ASkiSampleTrail::HeightAt(X)+(Ski?Rest.Z+5:5));
        const FVector Target=SkierVisual->GetRelativeTransform().InverseTransformPosition(Pose.UnrotateVector(SnowPoint-Origin-PivotLocation));
        const FQuat GroundRotation=FRotator(GetSlopeDegrees()+(Pole?90:0),Side*(Pole?30:5),0).Quaternion();
        const FQuat TargetRotation=Pose.Quaternion().Inverse()*GroundRotation*(Ski?Part.Rest.GetRotation():FQuat::Identity);
        const float Weight=Impact*(1-Stand);
        C->SetRelativeLocation(FMath::Lerp(C->GetRelativeLocation(),Target,Weight));
        C->SetRelativeRotation(FQuat::Slerp(C->GetRelativeRotation().Quaternion(),TargetRotation,Weight));
    }
    if(bAutomation&&Crashes==1)
    {
        const float CaptureTimes[]={.12f,.5f,1.2f};
        const TCHAR* CaptureNames[]={TEXT("FallImpact"),TEXT("FallSlide"),TEXT("FallGetUp")};
        if(FallCaptureStage<3&&Elapsed>CaptureTimes[FallCaptureStage])
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SkiQA/%s.png"),CaptureNames[FallCaptureStage]),true,false);
            ++FallCaptureStage;
        }
    }
    if(bAutomation&&!bCapturedFall&&Elapsed>.45f)
    {
        bCapturedFall=true;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SkiQA/Fall.png"),true,false);
    }
    if(FallRemaining<=0&&!Dead){ApplyBodyPose(.12f,0,0);BasePitch=VisualPitch=GetSlopeDegrees();}
}

bool ASkiRunnerCharacter::UpdatePresentationCamera(float Dt)
{
    const bool Dead=bFinished&&Hearts<=0;
    DeathBlend=FMath::FInterpTo(DeathBlend,Dead?1.f:0.f,Dt,1.4f);
    FollowCamera->PostProcessSettings.bOverride_SceneColorTint=true;
    FollowCamera->PostProcessSettings.SceneColorTint=FLinearColor(1.f,.82f,.80f);
    FollowCamera->PostProcessSettings.bOverride_VignetteIntensity=true;
    FollowCamera->PostProcessSettings.VignetteIntensity=.42f;
    FollowCamera->PostProcessBlendWeight=DeathBlend*.65f;
    if(bStarted&&!bFinished)return false;
    PresentationTime+=Dt;
    if(bFinished&&Hearts>0)
    {
        const float Close=FMath::SmoothStep(.25f,3.1f,PresentationTime);
        const float Out=FMath::SmoothStep(5.1f,6.8f,PresentationTime);
        const float Arm=FMath::Lerp(FMath::Lerp(1750.f,480.f,Close),1650.f,Out);
        const float Yaw=-90+12*FMath::Sin(FMath::Min(PresentationTime,5.5f)*.36f);
        CameraBoom->SetRelativeRotation(FMath::RInterpTo(CameraBoom->GetRelativeRotation(),FRotator(-4,Yaw,0),Dt,2));
        CameraBoom->TargetArmLength=FMath::FInterpTo(CameraBoom->TargetArmLength,Arm,Dt,3);
        CameraBoom->TargetOffset=FMath::VInterpTo(CameraBoom->TargetOffset,FVector(Out*500,0,35),Dt,2.5f);
        FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView,48.f,Dt,2));
        FollowCamera->SetRelativeLocation(FVector::ZeroVector);
        return true;
    }
    // Restrict the orbit to the authored side of the 2.5D set. Never circle
    // behind the mountains or allow the start/end cap into the frame.
    const float Phase=PresentationTime*(Dead?.38f:.28f);
    const float Yaw=-90+FMath::Sin(Phase)*(Dead?3.f:4.f);
    const float Pitch=-10+FMath::Sin(Phase*.7f)*(Dead?1.f:1.4f);
    const float Arm=Dead?FMath::Lerp(2200.f,1850.f,FMath::SmoothStep(0.f,3.5f,PresentationTime))+45*FMath::Sin(Phase):2050+100*FMath::Sin(Phase*.65f);
    CameraBoom->SetRelativeRotation(FMath::RInterpTo(CameraBoom->GetRelativeRotation(),FRotator(Pitch,Yaw,0),Dt,1.2f));
    CameraBoom->TargetArmLength=FMath::FInterpTo(CameraBoom->TargetArmLength,Arm,Dt,Dead?.8f:1.f);
    CameraBoom->TargetOffset=FMath::VInterpTo(CameraBoom->TargetOffset,FVector(Dead?520:500,0,Dead?90:130),Dt,1.1f);
    FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView,Dead?55.f:58.f,Dt,1.f));
    FollowCamera->SetRelativeLocation(FVector::ZeroVector);
    return true;
}

void ASkiRunnerCharacter::UpdateVictoryPose()
{
    const float Lift=FMath::SmoothStep(.65f,1.75f,PresentationTime);
    ApplyBodyPose(.03f,0,0);
    const float Turn=FMath::SmoothStep(.35f,1.65f,PresentationTime);
    VictoryLight->SetIntensity(1500*FMath::SmoothStep(.2f,1.5f,PresentationTime));
    const FRotator Pose(GetSlopeDegrees()*(1-Turn),65*Turn,0);
    TrickPivot->SetRelativeRotation(Pose);
    TrickPivot->SetRelativeLocation(FVector(0,0,-86)+Pose.RotateVector(FVector(0,0,80)));
    SkierVisual->SetRelativeScale3D(FVector::OneVector);
    PoseArms(.03f,0,0,Lift);
}

void ASkiRunnerCharacter::UpdatePresentationCheck(float Dt)
{
    const float Previous=PresentationQASeconds;PresentationQASeconds+=Dt;
    auto Capture=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("SkiQA/%s.png"),Name),true,false);};
    if(!bStarted)
    {
        QAYawMin=FMath::Min(QAYawMin,float(CameraBoom->GetRelativeRotation().Yaw));
        QAYawMax=FMath::Max(QAYawMax,float(CameraBoom->GetRelativeRotation().Yaw));
        if(Previous<1&&PresentationQASeconds>=1)Capture(TEXT("MenuEarly"));
        if(Previous<7&&PresentationQASeconds>=7)Capture(TEXT("MenuOrbit"));
        if(PresentationQASeconds>=8)StartOrJump();
    }
    if(bFinished&&Hearts<=0)
    {
        const float Old=DeathQASeconds;DeathQASeconds+=Dt;
        QADeathArmMin=FMath::Min(QADeathArmMin,CameraBoom->TargetArmLength);QADeathArmMax=FMath::Max(QADeathArmMax,CameraBoom->TargetArmLength);
        if(Old<.6f&&DeathQASeconds>=.6f)Capture(TEXT("DeathImpact"));
        if(Old<5&&DeathQASeconds>=5)Capture(TEXT("DeathOrbit"));
        if(DeathQASeconds>7)
        {
            const bool Passed=QAYawMax-QAYawMin>2&&QADeathArmMax-QADeathArmMin>100&&DeathBlend>.95f&&Hearts==0;
            const FString Report=FString::Printf(TEXT("{\"passed\":%s,\"menu_yaw_range\":%.2f,\"death_zoom_range\":%.2f,\"death_tint\":%.3f,\"hearts\":%d}"),Passed?TEXT("true"):TEXT("false"),QAYawMax-QAYawMin,QADeathArmMax-QADeathArmMin,DeathBlend,Hearts);
            FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("SkiQA/presentation.json")));
            UE_LOG(LogTemp,Display,TEXT("SKI_PRESENTATION_CHECK %s"),*Report);FPlatformMisc::RequestExit(false);
        }
    }
    if(PresentationQASeconds>150){FFileHelper::SaveStringToFile(TEXT("{\"passed\":false,\"reason\":\"timeout\"}"),*(FPaths::ProjectSavedDir()/TEXT("SkiQA/presentation.json")));FPlatformMisc::RequestExit(false);}
}
