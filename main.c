/*
 * powershell.exe - this program only calls pwsh.exe
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Commands that Waves Central calls, the last one fails but not fatal for Waves Central
 * powershell.exe Write-Host $PSVersionTable.PSVersion.Major $PSVersionTable.PSVersion.Minor
 * powershell.exe -command &{'{0}.{1}' -f $PSVersionTable.PSVersion.Major, $PSVersionTable.PSVersion.Minor}
 * powershell.exe (Get-PSDrive C).Free
 * powershell.exe -noLogo -noExit  -c Register-WMIEvent -Query 'SELECT * FROM Win32_DeviceChangeEvent WHERE (EventType = 2 OR EventType = 3) GROUP WITHIN 4'
 *                -Action { [System.Console]::WriteLine('Devices Changed') }
 *
 * Compile:
 * i686-w64-mingw32-gcc -municode  -mconsole main.c -lurlmon -luser32 -s -o powershell32.exe
 * x86_64-w64-mingw32-gcc -municode  -mconsole main.c -lurlmon -luser32 -s -o powershell64.exe
 */
#include <windows.h>
#include <stdio.h>

int __cdecl wmain( int argc, WCHAR *argv[] )
{
    BOOL no_psconsole = TRUE, noexit = FALSE;
    WCHAR conemu_pathW[MAX_PATH], cmdlineW[MAX_PATH]=L"", pwsh_pathW[MAX_PATH], bufW[MAX_PATH] = L" /i ";
    DWORD exitcode;       
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    int i = 1, j = 1;

    if( !ExpandEnvironmentStringsW( L"%ProgramW6432%", pwsh_pathW, MAX_PATH + 1 ) ) goto failed; /* win32 only apparently, not supported... */
    if( !ExpandEnvironmentStringsW( L"%SystemDrive%", conemu_pathW, MAX_PATH + 1 ) ) goto failed; 

    lstrcatW( conemu_pathW, L"\\ConEmu\\ConEmu.exe" );
    lstrcatW( pwsh_pathW, L"\\Powershell\\7\\pwsh.exe" );
    
    if ( GetFileAttributesW( pwsh_pathW ) == INVALID_FILE_ATTRIBUTES ) /* Download and install*/
    {
        WCHAR tmpW[MAX_PATH], profile_pathW[MAX_PATH], msiexecW[MAX_PATH];
    
        MessageBoxA(NULL, "Looks like Powershell Core is not installed \nWill start downloading and install now\n \
        This will take quite some time!!!\nNo progress bar is shown!", "Message", MB_ICONWARNING | MB_OK);
        
        if( !ExpandEnvironmentStringsW( L"%ProgramW6432%", profile_pathW, MAX_PATH + 1 ) ) goto failed; /* win32 only apparently, not supported... */
        if( !ExpandEnvironmentStringsW( L"%winsysdir%", msiexecW, MAX_PATH + 1 ) ) goto failed; 

        GetTempPathW( MAX_PATH, tmpW );
        fwprintf( stderr, L"\033[1;93m" ); fwprintf( stderr, L"\nDownloading %ls \n", L"PowerShell-7.0.3-win-x64.msi" ); fwprintf( stderr, L"\033[0m\n" );
        if( URLDownloadToFileW( NULL, L"https://github.com/PowerShell/PowerShell/releases/download/v7.0.3/PowerShell-7.0.3-win-x64.msi", lstrcatW( tmpW, L"PowerShell-7.0.3-win-x64.msi"), 0, NULL ) != S_OK )
            goto failed;
   
        GetTempPathW( MAX_PATH,tmpW );
        fwprintf( stderr, L"\033[1;93m" ); fwprintf( stderr, L"\nDownloading %ls \n", L"install2.ps1" ); fwprintf( stderr, L"\033[0m\n" );
        if( URLDownloadToFileW( NULL, L"https://conemu.github.io/install2.ps1", lstrcatW( tmpW, L"install2.ps1" ), 0, NULL ) != S_OK )
            goto failed;

        memset( &si, 0, sizeof( STARTUPINFO )); si.cb = sizeof( STARTUPINFO ); memset( &pi, 0, sizeof( PROCESS_INFORMATION ));
        GetTempPathW( MAX_PATH, tmpW );
        CreateProcessW(lstrcatW( msiexecW, L"\\msiexec.exe" ), lstrcatW(  lstrcatW( bufW, lstrcatW( tmpW, L"PowerShell-7.0.3-win-x64.msi") ), L" /q") , 0, 0, 0, 0, 0, 0, &si, &pi);
        WaitForSingleObject( pi.hProcess, INFINITE ); CloseHandle( pi.hProcess ); CloseHandle( pi.hThread );   

        memset( &si, 0, sizeof( STARTUPINFO ) ); si.cb = sizeof( STARTUPINFO ); memset( &pi , 0, sizeof( PROCESS_INFORMATION ) );
        GetTempPathW( MAX_PATH, tmpW ); bufW[0] = 0;
        CreateProcessW( pwsh_pathW, lstrcatW( lstrcatW( lstrcatW( bufW, L" -file " ), tmpW ), L"\\install2.ps1" ), 0, 0, 0, 0, 0, 0, &si, &pi);
        WaitForSingleObject( pi.hProcess, INFINITE ); CloseHandle( pi.hProcess ); CloseHandle( pi.hThread ); 
      
        fwprintf(stderr, L"\033[1;93m"); fwprintf(stderr, L"\nDownloading %ls \n", L"profile.ps1"); fwprintf(stderr, L"\033[0m\n");
        if( URLDownloadToFileW(NULL, L"https://raw.githubusercontent.com/PietJankbal/powershell-wrapper-for-wine/master/profile.ps1", lstrcatW( lstrcatW(profile_pathW, L"\\Powershell\\7\\"), L"profile.ps1"), 0, NULL) != S_OK )
            goto failed;
    } /* End download and install */

    BOOL is_single_or_last_option (WCHAR *opt)
    {
        return ( ( ( !_wcsnicmp( opt, L"-c", 2 ) && _wcsnicmp( opt, L"-config", 7 ) ) || !_wcsnicmp( opt, L"-n", 2 ) || \
                     !_wcsnicmp( opt, L"-m", 2 ) || !_wcsnicmp( opt, L"-s", 2 )  || !_wcsicmp( opt, L"-" ) || !_wcsnicmp( opt, L"-f", 2 )) ? TRUE : FALSE );
    }
    /* pwsh requires a command option "-c" , powershell doesn`t, so we have to insert it somewhere e.g. 'powershell -nologo 2+1' should go into 'powershell -nologo -c 2+1'*/ 
    while ( !_wcsnicmp(L"-", argv[i], 1 ) ) /* Search for 1st argument after options */
    {
        if ( !is_single_or_last_option ( argv[i] ) ) i++;
        i++;
    }

    if( i == argc) no_psconsole = FALSE;  /*no command found, start PSConsole later in ConEmu to work around bug https://bugs.winehq.org/show_bug.cgi?id=49780*/

    while (j < i ) /* concatenate options into new cmdline, meanwhile working around some incompabilities */ 
    {   
        if ( !_wcsnicmp( L"-noe", argv[j], 4 ) ) noexit = TRUE;    /* -NoExit, hack to start PSConsole in ConEmu later to work around bug https://bugs.winehq.org/show_bug.cgi?id=49780)*/
        if ( !_wcsnicmp( L"-f", argv[j], 2 ) ) no_psconsole = TRUE;/* -File, do not start in PSConsole */
        if ( !_wcsnicmp( L"-ve", argv[j], 3 ) ) {j++;  goto done;} /* -Version, exclude from new cmdline, incompatible... */
        if ( !_wcsnicmp( L"-nop", argv[j], 4 ) ) goto done;        /* -NoProfile, also exclude to always enable profile.ps1 to work around possible incompatibilities */   
        lstrcatW( lstrcatW( cmdlineW, L" " ), argv[j] );
        done: j++;
    }
    /* now insert a '-c' (if necessary) */
    if ( argv[i] && _wcsnicmp( argv[i-1], L"-c", 2 ) && _wcsicmp( argv[i-1], L"-" ) && _wcsnicmp( argv[i-1], L"-f", 2 ))
        lstrcatW( lstrcatW( cmdlineW, L" " ), L"-c " );

    while( i  < argc ) /* concatenate the rest of the arguments into the new cmdline */
    {
        lstrcatW( lstrcatW( cmdlineW, L" " ), argv[i] );
        i++;
    }
 
    if ( GetEnvironmentVariable( L"PWSHVERBOSE", bufW, MAX_PATH + 1 ) ) 
        { fwprintf( stderr, L"\033[1;35m" ); fwprintf( stderr, L"\n command line is %ls \n", cmdlineW ); fwprintf( stderr, L"\033[0m\n" ); }
    /* if not a command, start powershellconsole in ConEmu to work around missing ENABLE_VIRTUAL_TERMINAL_PROCESSING (bug https://bugs.winehq.org/show_bug.cgi?id=49780) */
    memset( &si, 0, sizeof( STARTUPINFO )); si.cb = sizeof( STARTUPINFO ); memset( &pi, 0, sizeof( PROCESS_INFORMATION ) );
    if( !no_psconsole || noexit)
    {
        bufW[0] = 0;
        CreateProcessW( conemu_pathW, lstrcatW( lstrcatW( lstrcatW( bufW, L" -resetdefault -Title \"This is Powershell Core (pwsh.exe), not (!) powershell.exe\" -run "), pwsh_pathW), cmdlineW), 0, 0, 0, 0, 0, 0, &si, &pi) ;
        WaitForSingleObject( pi.hProcess, INFINITE ); CloseHandle( pi.hProcess ); CloseHandle( pi.hThread );
        return 0;
    }
    /* replace incompatible commands/strings in the cmdline fed to pwsh.exe; see profile.ps1 howto replace */
    if ( GetEnvironmentVariableW( L"PSHACKS", bufW, MAX_PATH + 1 ) ) {
    /* Following function taken from https://creativeandcritical.net/downloads/replacebench.c which is in public domain; Credits to the there mentioned authors*/
    /* replaces in the string "str" all the occurrences of the string "sub" with the string "rep" */
    wchar_t* replace_smart (const wchar_t *str, const wchar_t *sub, const wchar_t *rep)
    {
        size_t slen = lstrlenW(sub); size_t rlen = lstrlenW(rep);
        size_t size = lstrlenW(str) + 1;
        size_t diff = rlen - slen;
        size_t capacity = (diff>0 && slen) ? 2 * size : size;
        wchar_t *buf = (wchar_t *)HeapAlloc(GetProcessHeap(),8,sizeof(wchar_t)*capacity);
        wchar_t *find, *b = buf;

        if (b == NULL) return NULL;
        if (slen == 0) return memcpy(b, str, sizeof(wchar_t)*size);

        while((find = /*strstrW*/(wchar_t *)wcsstr((const wchar_t *)str, (const wchar_t *)sub))) {
            if ((size += diff) > capacity) {
            wchar_t *ptr = (wchar_t *)HeapReAlloc(GetProcessHeap(), 0, buf, capacity = 2 * size*sizeof(wchar_t));
            if (ptr == NULL) {HeapFree(GetProcessHeap(), 0, buf); return NULL;}
            b = ptr + (b - buf);
            buf = ptr;
        }
        memcpy(b, str, (find - str) * sizeof(wchar_t)); /* copy up to occurrence */
        b += find - str;
        memcpy(b, rep, rlen * sizeof(wchar_t));       /* add replacement */
        b += rlen;
        str = find + slen;
        }
        memcpy(b, str, (size - (b - buf)) * sizeof(wchar_t));
        b = (wchar_t *)HeapReAlloc(GetProcessHeap(), 0, buf, size * sizeof(wchar_t));         /* trim to size */
        return b ? b : buf;
    }

    WCHAR buf_fromW[MAX_PATH];WCHAR buf_toW[MAX_PATH]; WCHAR *buf_replacedW=NULL;

    if (GetEnvironmentVariableW( L"PS_FROM", buf_fromW, MAX_PATH + 1 ) && GetEnvironmentVariableW( L"PS_TO", buf_toW, MAX_PATH + 1 )) {
        wchar_t *bufferA, *bufferB = 0;

        wchar_t* tokenA = wcstok_s(buf_fromW, L"¶", &bufferA); /* Use ¶ as separator, it will likely never show up in a command */
        wchar_t* tokenB = wcstok_s(buf_toW, L"¶", &bufferB);

        while (tokenA && tokenB) {
            buf_replacedW = replace_smart(cmdlineW, tokenA, tokenB);
            lstrcpyW( cmdlineW, buf_replacedW ); HeapFree(GetProcessHeap(), 0, buf_replacedW);

            tokenA = wcstok_s(NULL, L"¶", &bufferA);
            tokenB = wcstok_s(NULL, L"¶", &bufferB);
        }
    }
    }

    /* Otherwise execute the command through pwsh.exe */
    CreateProcessW( pwsh_pathW, cmdlineW , 0, 0, 0, 0, 0, 0, &si, &pi );
    WaitForSingleObject( pi.hProcess, INFINITE ); GetExitCodeProcess( pi.hProcess, &exitcode ); CloseHandle( pi.hProcess ); CloseHandle( pi.hThread );    

    return ( GetEnvironmentVariable( L"FAKESUCCESS", bufW, MAX_PATH + 1 ) ? 0 : exitcode ); 

failed:
    fprintf( stderr, "Something went wrong :( (32-bit?, winversion <win7?, failing download? ....  \n" );
    return 0; /* fake success anyway */
}
