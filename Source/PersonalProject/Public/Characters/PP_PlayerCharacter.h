// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "PP_BaseCharacter.h"
#include "Camera/CameraComponent.h"
#include "Characters/PP_CharacterData.h"
#include "PP_PlayerCharacter.generated.h"


class USpringArmComponent;
class UCameraComponent;

UCLASS()
class PERSONALPROJECT_API APP_PlayerCharacter : public APP_BaseCharacter
{
	GENERATED_BODY()

public:
	APP_PlayerCharacter();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

public:
	/** 根据角色ID应用外观（模型+动画） */
	void ApplyCharacterAppearance(FName CharacterId);

private:
	
	UPROPERTY(VisibleAnywhere,Category= "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;
	
	UPROPERTY(VisibleAnywhere,Category= "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** 角色数据表引用 - 用于根据 CharacterId 查找模型和动画 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Character")
	TObjectPtr<UDataTable> CharacterDataTable;
};
