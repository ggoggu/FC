import unreal

def create_or_get_asset(asset_name, package_path, asset_class, factory=None):
    full_path = f'{package_path}/{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        print(f'Asset exists: {full_path}')
        return unreal.EditorAssetLibrary.load_asset(full_path)
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = asset_tools.create_asset(asset_name, package_path, asset_class, factory)
    print(f'Created asset: {full_path}')
    return asset

# Setup DA_Card_Ignite
da_ignite_card = create_or_get_asset('DA_Card_Ignite', '/Game/Card', unreal.FCCardDataAsset, None)
if da_ignite_card:
    gp_data = da_ignite_card.get_editor_property('gameplay_data')
    gp_data.set_editor_property('card_id', unreal.Name('Card_Ignite'))
    gp_data.set_editor_property('base_mana_cost', 1)
    gp_data.set_editor_property('card_type', unreal.FCCardType.ATTACK)
    gp_data.set_editor_property('target_type', unreal.FCCardTargetType.ALL_ENEMIES)
    gp_data.set_editor_property('base_value', 10.0)
    gp_data.set_editor_property('required_class', unreal.FCCharacterClass.MAGE)
    gp_data.set_editor_property('elements', [unreal.FCElement.NONE])
    
    # Set ability class
    try:
        gp_data.set_editor_property('card_ability_class', unreal.FCGA_Ignite)
    except Exception as e:
        print(f'Warning setting card_ability_class directly: {e}')
        # Fallback via Class lookup
        ability_cls = unreal.load_class(None, '/Script/FC.FCGA_Ignite')
        if ability_cls:
            gp_data.set_editor_property('card_ability_class', ability_cls)

    da_ignite_card.set_editor_property('gameplay_data', gp_data)

    disp_data = da_ignite_card.get_editor_property('display_data')
    disp_data.set_editor_property('card_name', unreal.Text('점화'))
    disp_data.set_editor_property('card_description', unreal.Text('근처에 있는 모든 적에게 불스택 1개당 10의 피해를 입힙니다.'))
    disp_data.set_editor_property('rarity', unreal.FCCardRarity.UNCOMMON)
    da_ignite_card.set_editor_property('display_data', disp_data)

    unreal.EditorAssetLibrary.save_loaded_asset(da_ignite_card)
    print('SUCCESS: Configured and saved /Game/Card/DA_Card_Ignite')
else:
    print('ERROR: Failed to create or load DA_Card_Ignite')
