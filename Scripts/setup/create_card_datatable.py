import unreal
import os

def create_card_datatable():
    package_path = "/Game/Card"
    asset_name = "DT_CardCatalog"
    full_path = f"{package_path}/{asset_name}"

    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        print(f"[EXISTS] DataTable asset already exists: {full_path}")
        return True

    # 1. Resolve row struct /Script/FC.FCCardTableRow
    row_struct = unreal.load_object(None, "/Script/FC.FCCardTableRow")
    if not row_struct:
        print("[ERROR] Could not load /Script/FC.FCCardTableRow")
        return False

    # 2. Create DataTable asset with factory
    factory = unreal.DataTableFactory()
    factory.struct = row_struct

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    dt_asset = asset_tools.create_asset(asset_name, package_path, unreal.DataTable, factory)
    if not dt_asset:
        print(f"[ERROR] Failed to create DataTable asset: {full_path}")
        return False

    # 3. Read CSV data and populate DataTable
    csv_file_path = os.path.join(unreal.Paths.project_content_dir(), "Card", "DT_CardCatalog.csv")
    if os.path.exists(csv_file_path):
        with open(csv_file_path, "r", encoding="utf-8") as f:
            csv_content = f.read()

        # Try populating via DataTableFunctionLibrary or direct import
        b_success = False
        try:
            b_success = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_string(dt_asset, csv_content)
            print(f"[IMPORT] fill_data_table_from_csv_string result: {b_success}")
        except Exception as e:
            print(f"[WARN] fill_data_table_from_csv_string failed: {e}")

        if not b_success:
            try:
                b_success = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(dt_asset, csv_file_path)
                print(f"[IMPORT] fill_data_table_from_csv_file result: {b_success}")
            except Exception as e:
                print(f"[WARN] fill_data_table_from_csv_file failed: {e}")

    unreal.EditorAssetLibrary.save_loaded_asset(dt_asset)
    print(f"[SUCCESS] Created and saved DataTable: {full_path}")
    return True

if __name__ == "__main__":
    create_card_datatable()
