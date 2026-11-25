@echo off
set "MAKE_DIR=C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_2.2.0.202409170845\tools\bin"
set "GCC_DIR=C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344\tools\bin"
set "PATH=%MAKE_DIR%;%GCC_DIR%;%PATH%"

pushd "%~dp02.Firmware"
make %*
set ERR=%ERRORLEVEL%
popd
exit /b %ERR%
