nssm install ser2tcp "C:\Program Files\Serial to TCP\ser2tcp.exe"
REM nssm set ser2tcp AppDirectory """C:\Program Files\Serial to TCP\"""
nssm set ser2tcp AppParameters "config.toml"
nssm set ser2tcp DisplayName "Serial to TCP Bridge"
nssm set ser2tcp Description "Service to bridge serial traffic to a TCP connected client"
nssm set ser2tcp Start SERVICE_AUTO_START
nssm set ser2tcp ObjectName LocalSystem
nssm set ser2tcp AppStdout "C:\Program Files\Serial to TCP\ser2tcp-debug.log"
nssm set ser2tcp AppStderr "C:\Program Files\Serial to TCP\ser2tcp-errors.log"
nssm set ser2tcp AppStdoutCreationDisposition 4
nssm set ser2tcp AppRotateFiles 1
nssm set ser2tcp AppRotateOnline 1
nssm set ser2tcp AppRotateOnline 0
nssm set ser2tcp AppRotateBytes 1000000