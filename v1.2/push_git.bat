set "SRC=C:\Mercwar-AI\dc2win\dc2win_bk\dc2sql"
set "REPO_URL=https://github.com/mercwar/dc2sql"
set "STAGE=%TEMP%\dc2sql_push_v1_2"

rmdir /s /q "%STAGE%" 2>nul
mkdir "%STAGE%\v1.2"
robocopy "%SRC%" "%STAGE%\v1.2" /E /XD obj temp_vault .git /XF *.obj *.exp *.res *.log *.accdb *.laccdb /NFL /NDL /NJH /NJS /nc /ns /np >nul

git -C "%STAGE%" init -b main
git -C "%STAGE%" remote add origin "%REPO_URL%"
git -C "%STAGE%" fetch --depth=1 origin main
git -C "%STAGE%" reset --soft origin/main
git -C "%STAGE%" add "v1.2/"
git -C "%STAGE%" commit -m "Add DC2SQL v1.2 suite"
git -C "%STAGE%" push -u origin main
