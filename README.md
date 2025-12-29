# QtTranslator - 智能翻译字典

基于 Qt 6.9.2 开发的翻译工具，集成百度翻译 API。

## 核心功能
1. **在线翻译**：对接百度通用翻译 API，支持中英文互译。
2. **历史缓存**：使用 SQLite 数据库持久化存储查询记录。
3. **Model/View**：采用 QSqlTableModel 实现历史记录与界面的自动绑定。
4. **多线程优化**：使用 QtConcurrent 异步处理数据库写入，确保 UI 流畅。
5. **交互回显**：点击历史记录列表可快速查看翻译详情。

## 技术栈
- 框架：Qt 6.9.2 (Widgets, Network, Sql, Concurrent)
- 数据库：SQLite
- 构建工具：CMake