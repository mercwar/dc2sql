set "SRC=C:\Mercwar-AI\dc2win\dc2win_bk\dc2sql"
set "REPO_URL=https://github.com/mercwar/dc2sql"
set "STAGE=%TEMP%\dc2sql_push"

rmdir /s /q "%STAGE%" 2>nul
mkdir "%STAGE%"
robocopy "%SRC%" "%STAGE%" /E /XD obj temp_vault .git /XF *.obj *.exp *.res *.log *.accdb *.laccdb /NFL /NDL /NJH /NJS /nc /ns /np >nul

git -C "%STAGE%" init -b main
git -C "%STAGE%" remote add origin "%REPO_URL%"
git -C "%STAGE%" add -A
git -C "%STAGE%" commit -m "Push dc2sql"
git -C "%STAGE%" push -f -u origin main
