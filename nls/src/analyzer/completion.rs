use crate::analyzer::symbol::{NodeId, Symbol, SymbolKind, SymbolTable};
use crate::project::Module;
use log::debug;
use std::collections::HashSet;

#[derive(Debug, Clone)]
pub struct CompletionItem {
    pub label: String, // 变量名
    pub kind: CompletionItemKind,
    pub detail: Option<String>, // 类型信息
    pub documentation: Option<String>,
    pub insert_text: String,       // 插入文本
    pub sort_text: Option<String>, // 排序权重
}

#[derive(Debug, Clone)]
pub enum CompletionItemKind {
    Variable,
    Parameter,
    Function,
    Constant,
}

pub struct CompletionProvider<'a> {
    symbol_table: &'a mut SymbolTable,
    module: &'a mut Module,
}

impl<'a> CompletionProvider<'a> {
    pub fn new(symbol_table: &'a mut SymbolTable, module: &'a mut Module) -> Self {
        Self { symbol_table, module }
    }

    /// 主要的自动完成入口函数
    pub fn get_completions(&self, position: usize, prefix: &str) -> Vec<CompletionItem> {
        dbg!("get_completions", position, &self.module.ident, self.module.scope_id, prefix);

        // 检查是否在模块成员访问上下文中
        if let Some((imported_module, member_prefix)) = extract_module_member_context(prefix, position) {
            debug!("Detected module member access: {} and {}", imported_module, member_prefix);
            return self.get_module_member_completions(&imported_module, &member_prefix);
        }

        // 普通的变量补全
        let prefix = extract_prefix_at_position(prefix, position);
        debug!("Getting completions at position {} with prefix '{}'", position, prefix);

        // 1. 根据位置找到当前作用域
        let current_scope_id = self.find_innermost_scope(self.module.scope_id, position);
        debug!("Found scope_id {} by positon {}", current_scope_id, position);

        // 2. 收集所有可见的变量符号
        let mut completions = Vec::new();
        self.collect_variable_completions(current_scope_id, &prefix, &mut completions, position);

        // 3. 排序和过滤
        self.sort_and_filter_completions(&mut completions, &prefix);

        debug!("Found {} completions", completions.len());
        dbg!(&completions);

        completions
    }

    /// 获取模块成员的自动补全
    pub fn get_module_member_completions(&self, imported_as_name: &str, prefix: &str) -> Vec<CompletionItem> {
        debug!("Getting module member completions for module '{}' with prefix '{}'", imported_as_name, prefix);

        let mut completions = Vec::new();

        let deps = &self.module.dependencies;

        let import_stmt = deps.iter().find(|&dep| dep.as_name == imported_as_name);
        if import_stmt.is_none() {
            return completions;
        }

        let imported_module_ident = import_stmt.unwrap().module_ident.clone();
        debug!("Imported module is '{}' find by {}", imported_module_ident, imported_as_name);

        // 查找导入的模块作用域
        if let Some(&imported_scope_id) = self.symbol_table.module_scopes.get(&imported_module_ident) {
            let imported_scope = self.symbol_table.find_scope(imported_scope_id);

            // 遍历导入模块的所有符号
            for &symbol_id in &imported_scope.symbols {
                if let Some(symbol) = self.symbol_table.get_symbol_ref(symbol_id) {
                    // 只显示公开的符号（这里假设所有符号都是公开的，你可以根据需要添加可见性检查）
                    if prefix.is_empty() || symbol.ident.starts_with(prefix) {
                        let completion_item = self.create_module_completion_member(symbol);
                        debug!("Adding module member completion: {}", completion_item.label);
                        completions.push(completion_item);
                    }
                }
            }
        } else {
            debug!("Module '{}' not found in symbol table", imported_module_ident);
        }

        // 排序和过滤
        self.sort_and_filter_completions(&mut completions, prefix);

        debug!("Found {} module member completions", completions.len());
        completions
    }

    /// 从模块作用域开始，根据位置找到包含该位置的最内层作用域
    fn find_innermost_scope(&self, scope_id: NodeId, position: usize) -> NodeId {
        let scope = self.symbol_table.find_scope(scope_id);
        debug!("[find_innermost_scope] scope_id {}, start {}, end {}", scope_id, scope.range.0, scope.range.1);

        // 检查当前作用域是否包含该位置(range.1 == 0 表示整个文件级别的作用域)
        if position >= scope.range.0 && (position < scope.range.1 || scope.range.1 == 0) {
            // 检查子作用域，找到最内层的作用域
            for &child_id in &scope.children {
                let child_scope = self.symbol_table.find_scope(child_id);

                debug!(
                    "[find_innermost_scope] child scope_id {}, start {}, end {}",
                    scope_id, child_scope.range.0, child_scope.range.1
                );
                if position >= child_scope.range.0 && position < child_scope.range.1 {
                    return self.find_innermost_scope(child_id, position);
                }
            }

            return scope_id;
        }

        scope_id // 如果不在范围内，返回当前作用域
    }

    /// 收集变量完成项
    fn collect_variable_completions(&self, current_scope_id: NodeId, prefix: &str, completions: &mut Vec<CompletionItem>, position: usize) {
        let mut visited_scopes = HashSet::new();
        let mut current = current_scope_id;

        // 从当前作用域向上遍历
        while current > 0 && !visited_scopes.contains(&current) {
            visited_scopes.insert(current);

            let scope = self.symbol_table.find_scope(current);
            debug!("Searching scope {} with {} symbols", current, scope.symbols.len());

            // 遍历当前作用域的所有符号
            for &symbol_id in &scope.symbols {
                if let Some(symbol) = self.symbol_table.get_symbol_ref(symbol_id) {
                    dbg!("Found symbol will check", symbol.ident.clone(), prefix, symbol.ident.starts_with(prefix));

                    // 只处理变量和常量
                    match &symbol.kind {
                        SymbolKind::Var(_) | SymbolKind::Const(_) => {
                            if (prefix.is_empty() || symbol.ident.starts_with(prefix)) && symbol.pos < position {
                                let completion_item = self.create_completion_item(symbol);
                                debug!("Adding completion: {}", completion_item.label);
                                completions.push(completion_item);
                            }
                        }
                        _ => {}
                    }
                }
            }
            current = scope.parent;

            // 如果到达了根作用域，停止遍历
            if current == 0 {
                break;
            }
        }
    }

    /// 创建完成项
    fn create_completion_item(&self, symbol: &Symbol) -> CompletionItem {
        let (kind, detail) = match &symbol.kind {
            SymbolKind::Var(var_decl) => {
                let detail = {
                    let var = var_decl.lock().unwrap();
                    format!("var: {}", var.type_)
                };
                (CompletionItemKind::Variable, Some(detail))
            }
            SymbolKind::Const(const_def) => {
                let detail = {
                    let const_val = const_def.lock().unwrap();
                    format!("const: {}", const_val.type_)
                };
                (CompletionItemKind::Constant, Some(detail))
            }
            _ => (CompletionItemKind::Variable, None),
        };

        CompletionItem {
            label: symbol.ident.clone(),
            kind,
            detail,
            documentation: None,
            insert_text: symbol.ident.clone(),
            sort_text: Some(format!("{:08}", symbol.pos)), // 按定义位置排序
        }
    }

    /// 创建模块成员完成项
    fn create_module_completion_member(&self, symbol: &Symbol) -> CompletionItem {
        let (ident, kind, detail) = match &symbol.kind {
            SymbolKind::Var(var) => {
                let var = var.lock().unwrap();
                let detail = format!("var: {}", var.type_);
                let display_ident = extract_last_ident_part(&var.ident.clone());
                (display_ident, CompletionItemKind::Variable, Some(detail))
            }
            SymbolKind::Const(constdef) => {
                let constdef = constdef.lock().unwrap();
                let detail = format!("const: {}", constdef.type_);
                let display_ident = extract_last_ident_part(&constdef.ident.clone());
                (display_ident, CompletionItemKind::Constant, Some(detail))
            }
            SymbolKind::Fn(fndef) => {
                let fndef = fndef.lock().unwrap();
                let detail =  format!("fn: {}", fndef.type_);
                (fndef.fn_name.clone(), CompletionItemKind::Function, Some(detail))
            }
            SymbolKind::Type(typedef) => {
                let typedef = typedef.lock().unwrap();
                let detail = format!("type definition");
                let display_ident = extract_last_ident_part(&typedef.ident);
                (display_ident, CompletionItemKind::Variable, Some(detail))
            }
        };

        CompletionItem {
            label: ident.clone(),
            kind,
            detail,
            documentation: None,
            insert_text: ident.clone(),
            sort_text: Some(format!("{:08}", symbol.pos)),
        }
    }

    /// 排序和过滤完成项
    fn sort_and_filter_completions(&self, completions: &mut Vec<CompletionItem>, prefix: &str) {
        // 去重 - 基于标签去重
        completions.sort_by(|a, b| a.label.cmp(&b.label));
        completions.dedup_by(|a, b| a.label == b.label);

        // 按匹配度和定义位置排序
        completions.sort_by(|a, b| {
            // 精确前缀匹配优先
            let a_exact = a.label.starts_with(prefix);
            let b_exact = b.label.starts_with(prefix);

            match (a_exact, b_exact) {
                (true, false) => std::cmp::Ordering::Less,
                (false, true) => std::cmp::Ordering::Greater,
                _ => {
                    // 按字母顺序排序
                    a.label.cmp(&b.label)
                }
            }
        });

        // 限制返回数量
        completions.truncate(50);
    }
}

/// 从文本中提取光标位置的前缀
pub fn extract_prefix_at_position(text: &str, position: usize) -> String {
    if position == 0 {
        return String::new();
    }

    let chars: Vec<char> = text.chars().collect();
    if position > chars.len() {
        return String::new();
    }

    let mut start = position;

    // 向前查找标识符的开始位置
    while start > 0 {
        let ch = chars[start - 1];
        if ch.is_alphanumeric() || ch == '_' || ch == '.' {
            start -= 1;
        } else {
            break;
        }
    }

    // 提取前缀
    chars[start..position].iter().collect()
}

/// 从类似 "io.main.writer" 的标识符中提取最后一个部分 "writer"
fn extract_last_ident_part(ident: &str) -> String {
    if let Some(dot_pos) = ident.rfind('.') {
        ident[dot_pos + 1..].to_string()
    } else {
        ident.to_string()
    }
}

/// 检测是否在模块成员访问上下文中，返回 (模块名, 成员前缀)
pub fn extract_module_member_context(prefix: &str, _position: usize) -> Option<(String, String)> {
    if let Some(dot_pos) = prefix.rfind('.') {
        let module_name = prefix[..dot_pos].to_string();
        let member_prefix = prefix[dot_pos + 1..].to_string();
        
        if !module_name.is_empty() {
            return Some((module_name, member_prefix));
        }
    }
    
    None
}
