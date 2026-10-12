#include "SkiRunnerCharacter.h"
#include "SkiSampleTrail.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameViewportClient.h"
#include "Misc/Paths.h"

void ASkiRunnerCharacter::UpdateSkiSpray(float Dt)
{
    if(SkiSnow.IsEmpty())return;
    const auto* Floor=GetCharacterMovement()->CurrentFloor.HitResult.GetComponent();
    const bool OnBareObstacle=Floor&&Floor->GetCollisionObjectType()==ECC_GameTraceChannel1;
    const bool Emit=!OnBareObstacle&&bStarted&&!bFinished&&GetCharacterMovement()->IsMovingOnGround()&&GetVelocity().SizeSquared()>FMath::Square(180.f);
    if(Emit)
    {
        const float Intensity=(bBraking?1.5f:1.f)*(FallRemaining>0?1.8f:1.f);
        SprayAccumulator+=Dt*(50+ForwardSpeed*.025f)*Intensity;
        const int32 Count=FMath::Min(FMath::FloorToInt(SprayAccumulator),32);
        SprayAccumulator-=Count;
        const FVector Tangent=FRotator(GetSlopeDegrees(),0,0).Vector();
        for(int32 I=0;I<Count;++I)
        {
            FSkiSnow& Particle=SkiSnow[NextSpray];
            Particle.Position=SkierVisual->GetComponentTransform().TransformPosition(FVector(-85,(NextSpray%2?17:-17),8));
            // Spread births over the last travelled segment, avoiding clumps at low FPS.
            Particle.Position-=GetVelocity()*Dt*SprayRandom.FRand();
            Particle.Position.Z=FMath::Max(Particle.Position.Z,ASkiSampleTrail::SurfaceHeightAt(Particle.Position.X)+8);
            Particle.Velocity=-Tangent*SprayRandom.FRandRange(120,330)+FVector(0,SprayRandom.FRandRange(-100,100),SprayRandom.FRandRange(100,280))*Intensity;
            Particle.Age=0;Particle.Life=SprayRandom.FRandRange(.38f,.75f);
            Particle.Size=SprayRandom.FRandRange(.07f,.19f)*Intensity;
            NextSpray=(NextSpray+1)%SkiSnow.Num();++SprayEmitted;
        }
    }
    else SprayAccumulator=0;
    for(int32 I=0;I<SkiSnow.Num();++I)
    {
        FSkiSnow& Particle=SkiSnow[I];
        if(Particle.Life<=0)continue;
        Particle.Age+=Dt;
        if(Particle.Age>=Particle.Life)
        {
            SkiSpray->UpdateInstanceTransform(I,FTransform(FRotator::ZeroRotator,Particle.Position,FVector::ZeroVector),true,false,true);
            Particle.Life=0;continue;
        }
        Particle.Velocity.Z-=640*Dt;Particle.Position+=Particle.Velocity*Dt;
        const float Ground=ASkiSampleTrail::SurfaceHeightAt(Particle.Position.X)+5;
        if(Particle.Position.Z<Ground){Particle.Position.Z=Ground;Particle.Velocity*=.6f;Particle.Velocity.Z=0;}
        const float Fade=1-Particle.Age/Particle.Life;
        const float Scale=Particle.Size*FMath::Sqrt(Fade);
        SkiSpray->UpdateInstanceTransform(I,FTransform(Particle.Velocity.Rotation(),Particle.Position,FVector(Scale*1.6f,Scale,Scale*.6f)),true,false,true);
    }
    SkiSpray->MarkRenderStateDirty();
    if(bAutomation&&!bNoInputTest&&!bCapturedSpray&&Emit&&SprayEmitted>180&&Time>3)
    {
        bCapturedSpray=true;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("SkiQA/SkiSpray.png"),true,false);
    }
}
