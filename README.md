# JsonCpp
使用`C++`开发的一个`Json`解析器，提供对`Json`字符串的快速解析。*仅为学习之作*.

## LICENSE
源代码在`MIT`协议下发布。具体内容要求见`LICENSE`文件。

## 简要说明
本解析器依赖于异常以及`STL`，解析类将保证异常安全，对于静态方法，保证线程安全，但是实例方法，目前不保证线程安全，不过现在只有在多线程中对于`ToString`实例方法的访问有可能发生竞争险象，其它公开方法均不会影响改变对象状态。

目前尚未加入更多序列化以及反序列化方法。

### 关于解析
解析过程中采用快进式的读取解析方式，基本只需要对字符串扫描一遍即可，对于细节部分，还需要继续优化，比如对于**C++11**中引入的`move`构造语法的利用等。

## 历程
* `v0.1.0`版本已经基本实现了从`Json`字符串到`Json`对象的解析，除了继续统一接口规范之外，准备逐渐加上对`JPath`语法的支持。
* `v0.2.0-beta`：略…
* `v0.3.0`版基本完成对普通`JPath`语法的支持，有关具体支持的语法形式，将在下文**JsonPath**一节给出示例样式。

## JsonPath
类似`XPath`（`XmlPath`）语法，对于`Json`数据对象，也同样有一套路径访问语法，称为`JsonPath`表达式，也称为`JPath`。具体详细语法规则可以参见[JsonPath语法](http://goessner.net/articles/JsonPath/)。下面给出几个示例：

``` JavaScript
// 点标记语法
$.store.book[0].title
// 括号标记语法
$['store']['book'][0]['title']
// 使用索引
$.store.book[(@.length-1)].title
// 过滤器
$.store.book[?(@.price < 10)].title
```

其它更多详细规则，可以从上面链接网址中查看，同时还可以查看与`XPath`语法的类比。

### 支持语法
目前解析器**仅**支持使用点标记语法进行选择`token`以及`token`列表，除了对数组的部分语法支持不完整之外，其它基本语法均支持，包括递归模式和普通模式等。有关这一部分的支持细节，也会在下面提到，因为时间原因，尚未观察其它类似解析器对`JPath`语法的解析形式，而标准文档在细节方面也不甚详细，因此有少数语法不一定符合标准要求（主要是对符号**$**的解析），而有些语法可能是标准语法的扩展（主要是递归模式下解析方式）。

对于标准语法中，数组的下标可以使用表达式以及过滤器等样式，而这些需要`JavaScript`解释器，因此对于此处的实现，目前仅作简单的支持，主要是数值类型的计算（包括布尔运算）。具体内容罗列如下（未完待续...）

## 文档与说明
相关文档和说明内容正在整理中…  
<!-- 要过年了 -->
