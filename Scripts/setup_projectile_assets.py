import unreal

def create_or_get_asset(asset_name, package_path, asset_class, factory):
    full_path = f'{package_path}/{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        print(f'Asset exists: {full_path}')
        return unreal.EditorAssetLibrary.load_asset(full_path)
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = asset_tools.create_asset(asset_name, package_path, asset_class, factory)
    print(f'Created asset: {full_path}')
    return asset

# 1. Create BP_FCProjectile_Arrow & BP_FCProjectile_Orb
bp_factory = unreal.BlueprintFactory()
bp_factory.set_editor_property('parent_class', unreal.FCProjectileBase)

bp_arrow = create_or_get_asset('BP_FCProjectile_Arrow', '/Game/Combat/BP/Template', unreal.Blueprint, bp_factory)
if bp_arrow:
    unreal.EditorAssetLibrary.save_loaded_asset(bp_arrow)
    print('Saved BP_FCProjectile_Arrow')

bp_orb = create_or_get_asset('BP_FCProjectile_Orb', '/Game/Combat/BP/Template', unreal.Blueprint, bp_factory)
if bp_orb:
    unreal.EditorAssetLibrary.save_loaded_asset(bp_orb)
    print('Saved BP_FCProjectile_Orb')

# 2. Setup DA_Projectile_FireArrow
tools = unreal.AssetToolsHelpers.get_asset_tools()
da_firearrow_proj = create_or_get_asset('DA_Projectile_FireArrow', '/Game/Combat/Data', unreal.FCProjectileDataAsset, None)
if da_firearrow_proj:
    if bp_arrow:
        da_firearrow_proj.set_editor_property('projectile_class', bp_arrow.generated_class())
    da_firearrow_proj.set_editor_property('damage', 1.0)
    da_firearrow_proj.set_editor_property('launch_speed', 3000.0)
    da_firearrow_proj.set_editor_property('max_speed', 3000.0)
    da_firearrow_proj.set_editor_property('gravity_scale', 0.0)
    da_firearrow_proj.set_editor_property('projectile_elements', [unreal.FCElement.FIRE])
    da_firearrow_proj.set_editor_property('source_class', unreal.FCCharacterClass.MAGE)
    da_firearrow_proj.set_editor_property('source_card_type', unreal.FCCardType.ATTACK)
    da_firearrow_proj.set_editor_property('life_span', 5.0)
    unreal.EditorAssetLibrary.save_loaded_asset(da_firearrow_proj)
    print('Configured DA_Projectile_FireArrow with BP_FCProjectile_Arrow')

# 3. Setup DA_Projectile_Fireball
da_fireball_proj = create_or_get_asset('DA_Projectile_Fireball', '/Game/Combat/Data', unreal.FCProjectileDataAsset, None)
if da_fireball_proj:
    if bp_orb:
        da_fireball_proj.set_editor_property('projectile_class', bp_orb.generated_class())
    da_fireball_proj.set_editor_property('damage', 1.0)
    da_fireball_proj.set_editor_property('launch_speed', 2500.0)
    da_fireball_proj.set_editor_property('max_speed', 2500.0)
    da_fireball_proj.set_editor_property('gravity_scale', 0.0)
    da_fireball_proj.set_editor_property('projectile_elements', [unreal.FCElement.FIRE, unreal.FCElement.EARTH])
    da_fireball_proj.set_editor_property('source_class', unreal.FCCharacterClass.MAGE)
    da_fireball_proj.set_editor_property('source_card_type', unreal.FCCardType.ATTACK)
    da_fireball_proj.set_editor_property('life_span', 5.0)
    unreal.EditorAssetLibrary.save_loaded_asset(da_fireball_proj)
    print('Configured DA_Projectile_Fireball with BP_FCProjectile_Orb')

# 4. Setup DA_Card_FireArrow
da_firearrow_card = create_or_get_asset('DA_Card_FireArrow', '/Game/Card', unreal.FCCardDataAsset, None)
if da_firearrow_card:
    gp_data = da_firearrow_card.get_editor_property('gameplay_data')
    gp_data.set_editor_property('card_id', unreal.Name('Card_FireArrow'))
    gp_data.set_editor_property('base_mana_cost', 1)
    gp_data.set_editor_property('card_type', unreal.FCCardType.ATTACK)
    gp_data.set_editor_property('target_type', unreal.FCCardTargetType.DIRECTIONAL_AO_E)
    gp_data.set_editor_property('base_value', 1.0)
    gp_data.set_editor_property('required_class', unreal.FCCharacterClass.MAGE)
    gp_data.set_editor_property('elements', [unreal.FCElement.FIRE])
    gp_data.set_editor_property('card_ability_class', unreal.FCGA_SpawnProjectile)
    gp_data.set_editor_property('projectile_data_asset', da_firearrow_proj)
    da_firearrow_card.set_editor_property('gameplay_data', gp_data)

    disp_data = da_firearrow_card.get_editor_property('display_data')
    disp_data.set_editor_property('card_name', unreal.Text('파이어 에로우'))
    disp_data.set_editor_property('card_description', unreal.Text('불 화살을 발사한다.'))
    disp_data.set_editor_property('rarity', unreal.FCCardRarity.COMMON)
    da_firearrow_card.set_editor_property('display_data', disp_data)

    unreal.EditorAssetLibrary.save_loaded_asset(da_firearrow_card)
    print('Configured and saved DA_Card_FireArrow')

print('ALL ASSETS CONFIGURED SUCCESSFULLY FOR OPTION 1 ROLE SEPARATION!')
