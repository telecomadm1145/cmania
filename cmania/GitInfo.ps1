# 获取最近的标签
$latestTag = & git describe --abbrev=0 --tags
$gitCommitHash = & git rev-parse --short HEAD
$gitCommitDate = & git log -1 --format=%cd --date=format:"%Y-%m-%d %H:%M:%S"

# 构建宏定义字符串
$macroDefinition = "#pragma once"
$macroDefinition += "`n`n#define GIT_LATEST_TAG `"$latestTag`""
$macroDefinition += "`n#define GIT_COMMIT_HASH `"$gitCommitHash`""
$macroDefinition += "`n#define GIT_COMMIT_DATE `"$gitCommitDate`""

# 将宏定义写入文件
$macroDefinition | Out-File -FilePath "git_info.h" -Encoding utf8