param(
    [string]$SourcePath = "$PSScriptRoot/../src/mbedtls.c",
    [string[]]$Cases = @('encoding', 'lifecycle', 'derivation')
)
$ErrorActionPreference = 'Stop'
$source = [IO.File]::ReadAllText((Resolve-Path $SourcePath))
$template = [IO.File]::ReadAllText("$PSScriptRoot/rsa_regression.c")
$functions = @(
    @('int', '_libssh2_mbedtls_rsa_new_private_frommemory'),
    @('static unsigned char *', 'gen_publickey_from_rsa'),
    @('static int', '_libssh2_mbedtls_pub_priv_key')
)
$extracted = foreach ($function in $functions) {
    $pattern = '(?ms)^' + [regex]::Escape($function[1]) + '\(.*?^\}'
    $match = [regex]::Match($source, $pattern)
    if (!$match.Success) { throw "Missing function: $($function[1])" }
    $function[0] + "`n" + $match.Value
}
$directory = Join-Path ([IO.Path]::GetTempPath()) ([guid]::NewGuid().ToString())
[IO.Directory]::CreateDirectory($directory) | Out-Null
try {
    $inputFile = Join-Path $directory 'rsa_test.c'
    [IO.File]::WriteAllText($inputFile,
        $template.Replace('/* BACKEND_FUNCTIONS */', ($extracted -join "`n")))
    foreach ($version in @('0x021C0000', '0x03060000')) {
        $exe = Join-Path $directory 'rsa_test.exe'
        # Pattern initialization makes an uninitialized success result fail
        # deterministically; guard bytes detect the one-byte output overflow.
        & gcc -std=c11 -O0 -Wall -Wextra -ftrivial-auto-var-init=pattern "-DMBEDTLS_VERSION_NUMBER=$version" $inputFile -o $exe
        if ($LASTEXITCODE) { throw 'Host compilation failed' }
        foreach ($case in $Cases) {
            Write-Host "mbedTLS API $version / $case"
            & $exe $case
            if ($LASTEXITCODE) { throw "Regression failed: $case" }
        }
    }
}
finally {
    # Delete only the known files created in this unique temporary directory.
    foreach ($name in @('rsa_test.c', 'rsa_test.exe')) {
        $path = Join-Path $directory $name
        if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path }
    }
    [IO.Directory]::Delete($directory)
}
