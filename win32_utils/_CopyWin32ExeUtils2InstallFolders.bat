@echo off
Break ON
echo This batch file copies the 13 win32_utils folder's .exe 
echo files to each of the following installation folders for 
echo incorporation into these Inno Setup script installers:
echo     1. Setup Unicode
echo     2. Setup Unicode - Minimal
echo     3. setup Unicode - No Html Help
echo ===================================================
echo The names of the Windows exe files are:
echo do_add_KBUsers.exe
echo do_change_fullname.exe
echo do_change_password.exe
echo do_change_permission.exe
echo do_changed_since_timed.exe
echo do_create_entry.exe
echo do_list_users.exe
echo do_lookup_entry.exe
echo do_pseudo_delete.exe
echo do_pseudo_undelete.exe
echo do_upload_local_kb.exe
echo do_user_lookup.exe
echo do_users_list.exe
echo ===================================================

@echo on
rem 1. The following copies the win32_utils exe files to the "setup Unicode" folder
@echo off
mkdir "..\setup Unicode\" "..\setup Unicode\"
copy "do_add_KBUsers.exe" "..\setup Unicode\"
copy "do_change_fullname.exe" "..\setup Unicode\"
copy "do_change_password.exe" "..\setup Unicode\"
copy "do_change_permission.exe" "..\setup Unicode\"
copy "do_changed_since_timed.exe" "..\setup Unicode\"
copy "do_create_entry.exe" "..\setup Unicode\"
copy "do_list_users.exe" "..\setup Unicode\"
copy "do_lookup_entry.exe" "..\setup Unicode\"
copy "do_pseudo_delete.exe" "..\setup Unicode\"
copy "do_pseudo_undelete.exe" "..\setup Unicode\"
copy "do_upload_local_kb.exe" "..\setup Unicode\"
copy "do_user_lookup.exe" "..\setup Unicode\"
copy "do_users_list.exe" "..\setup Unicode\"

@echo on
rem 2. The following copies win32_utils exe  files to the "setup Unicode - Minimal" folder
@echo off
mkdir "..\setup Unicode - Minimal\"
copy "do_add_KBUsers.exe" "..\setup Unicode - Minimal\"
copy "do_change_fullname.exe" "..\setup Unicode - Minimal\"
copy "do_change_password.exe" "..\setup Unicode - Minimal\"
copy "do_change_permission.exe" "..\setup Unicode - Minimal\"
copy "do_changed_since_timed.exe" "..\setup Unicode - Minimal\"
copy "do_create_entry.exe" "..\setup Unicode - Minimal\"
copy "do_list_users.exe" "..\setup Unicode - Minimal\"
copy "do_lookup_entry.exe" "..\setup Unicode - Minimal\"
copy "do_pseudo_delete.exe" "..\setup Unicode - Minimal\"
copy "do_pseudo_undelete.exe" "..\setup Unicode - Minimal\"
copy "do_upload_local_kb.exe" "..\setup Unicode - Minimal\"
copy "do_user_lookup.exe" "..\setup Unicode - Minimal\"
copy "do_users_list.exe" "..\setup Unicode - Minimal\"

@echo on
rem 3. The following copies win32_utils exe  files to the "setup Unicode - No Html Help" folder
@echo off
mkdir "..\setup Unicode - No Html Help\"
copy "do_add_KBUsers.exe" "..\setup Unicode - No Html Help\"
copy "do_change_fullname.exe" "..\setup Unicode - No Html Help\"
copy "do_change_password.exe" "..\setup Unicode - No Html Help\"
copy "do_change_permission.exe" "..\setup Unicode - No Html Help\"
copy "do_changed_since_timed.exe" "..\setup Unicode - No Html Help\"
copy "do_create_entry.exe" "..\setup Unicode - No Html Help\"
copy "do_list_users.exe" "..\setup Unicode - No Html Help\"
copy "do_lookup_entry.exe" "..\setup Unicode - No Html Help\"
copy "do_pseudo_delete.exe" "..\setup Unicode - No Html Help\"
copy "do_pseudo_undelete.exe" "..\setup Unicode - No Html Help\"
copy "do_upload_local_kb.exe" "..\setup Unicode - No Html Help\"
copy "do_user_lookup.exe" "..\setup Unicode - No Html Help\"
copy "do_users_list.exe" "..\setup Unicode - No Html Help\"

echo ===================================================
echo Copy process completed.
