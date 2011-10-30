#pragma once

#include <set>

#include <Poco/SharedPtr.h>

#include <DB/Parsers/IAST.h>
#include <DB/Interpreters/Context.h>
#include <DB/Interpreters/Aggregator.h>


namespace DB
{

/** Интерпретирует выражение из синтаксического дерева,
  *  в котором присутствуют имена столбцов, константы и обычные функции.
  */
class Expression
{
public:
	Expression(ASTPtr ast_, const Context & context_) : ast(ast_), context(context_)
	{
		addSemantic(ast);
		glueTree(ast);
	}

	/** Получить список столбцов, которых необходимо прочитать из таблицы, чтобы выполнить выражение.
	  */
	Names getRequiredColumns();

	/** Прописать во всех узлах, что они ещё не вычислены.
	  * Вызывайте в начале серии вычислений, для каждого блока.
	  */
	void setNotCalculated(unsigned part_id = 0, ASTPtr subtree = NULL);

	/** Выполнить выражение над блоком. Блок должен содержать все столбцы - идентификаторы.
	  * Функция добавляет в блок новые столбцы - результаты вычислений.
	  * part_id - какую часть выражения вычислять.
	  */
	void execute(Block & block, unsigned part_id = 0);

	/** Взять из блока с промежуточными результатами вычислений только столбцы, представляющие собой конечный результат.
	  * Вернуть новый блок, в котором эти столбцы расположены в правильном порядке.
	  */
	Block projectResult(Block & block, bool without_duplicates = false, unsigned part_id = 0, ASTPtr subtree = NULL);

	/** Получить список типов столбцов результата.
	  */
	DataTypes getReturnTypes();

	/** Получить блок-образец, содержащий имена и типы столбцов результата.
	  */
	Block getSampleBlock();

	/** Получить список ключей агрегирования и описаний агрегатных функций, если в запросе есть GROUP BY.
	  */
	void getAggregateInfo(Names & key_names, AggregateDescriptions & aggregates);

	/** Есть ли в выражении агрегатные функции.
	  */
	bool hasAggregates();

	/** Пометить то, что должно быть вычислено до агрегирования одним part_id,
	  * а то, что должно быть вычислено после агрегирования, а также сами агрегатные функции - другим part_id.
	  */
	void markBeforeAndAfterAggregation(unsigned before_part_id, unsigned after_part_id);

private:
	ASTPtr ast;
	const Context & context;

	typedef std::set<String> NamesSet;
	NamesSet required_columns;
	
	
	/** Для узлов - звёздочек - раскрыть их в список всех столбцов.
	  * Для узлов - литералов - прописать их типы данных.
	  * Для узлов - функций - прописать ссылки на функции, заменить имена на канонические, прописать и проверить типы.
	  * Для узлов - идентификаторов - прописать ссылки на их типы.
	  * Проверить, что все функции применимы для типов их аргументов.
	  */
	void addSemantic(ASTPtr & ast);

	/** Склеить одинаковые узлы в синтаксическом дереве (превращая его в направленный ациклический граф).
	  * Это означает, в том числе то, что функции с одними и теми же аргументами, будут выполняться только один раз.
	  * Например, выражение rand(), rand() вернёт два идентичных столбца.
	  * Поэтому, все функции, в которых такое поведение нежелательно, должны содержать дополнительный параметр "тег".
	  */
	void glueTree(ASTPtr ast);

	typedef std::map<String, ASTPtr> Subtrees;
	
	void glueTreeImpl(ASTPtr ast, Subtrees & Subtrees);

	void executeImpl(ASTPtr ast, Block & block, unsigned part_id);

	void collectFinalColumns(ASTPtr ast, Block & src, Block & dst, bool without_duplicates, unsigned part_id);

	void getReturnTypesImpl(ASTPtr ast, DataTypes & res);

	void getSampleBlockImpl(ASTPtr ast, Block & res);

	void getAggregateInfoImpl(ASTPtr ast, Names & key_names, AggregateDescriptions & aggregates, NamesSet & processed);

	bool hasAggregatesImpl(ASTPtr ast);

	void markBeforeAndAfterAggregationImpl(ASTPtr ast, unsigned before_part_id, unsigned after_part_id, bool below = false);
};


}
