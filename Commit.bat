@ECHO off

SET /p Message="Enter A commit message: "

git add *
git commit -m "%Message%"

IF %ERRORLEVEL% NEQ 0 PAUSE
