Param(
    [Parameter(Mandatory = $false)]
    [Switch]
    $Check
)

function Invoke-ClangFormat() {
    $files = Get-ChildItem "src" -Exclude "Debug","Release" |
        Get-ChildItem -Recurse |
        Where-Object { $_.Extension -match ".cpp|.h" } |
        Select-Object -ExpandProperty FullName

    if ($Check) {
        $out = & "./tools/clang-format.exe" -output-replacements-xml $files 2>&1
        if ($out -match "<replacement ") {
            Write-Error "Incorrect code formatting, run './tools/codeformat.ps1'"
            exit 1
        }
    } else {
        & "./tools/clang-format.exe" -i $files
    }
}

Invoke-ClangFormat