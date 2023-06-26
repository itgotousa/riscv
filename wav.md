
## BWF文件格式规范

BWF文件格式是在SWF文件格式的基础上进行少量优化而产生的专门用于教育视频的格式规范。 SWF文件格式规范请参考：SWF File Format Specification Version 9

BWF文件的整体结构包括一个16字节的文件头，和后面一些列的Tag。

### BWF文件的文件头(file header)
BWF文件的开始个字节的含义如下：
- 第0个字节 ： 为ASCII码 B
- 第1个字节 ： 为ASCII码 W
- 第2个字节 ： 为ASCII码 F
- 第3个字节 ： 为文件格式的版本号，目前为1
- 第4到第7共4个字节 ： 此四个字节记录了整个文件的大小，单位是字节，包含文件头。
- 第8到第9共2个字节 ： 视频的宽度，单位是像素pixel
- 第10到第11共2个字节 ：视频的高度，单位是像素pixel
- 第12个字节 ：保留，随机产生一个数字。
- 第14个字节 ：每秒钟播放的帧数
- 第14到第15共两个字节：帧的总数。

### BWF文件的Tag
第15个字节是第一个Tag的起始位置。BWF共计支持如下10个Tag类型：
- ST_TITLE
- ST_KEYWORD
- ST_SOUNDSTREAMHEAD2
- ST_SOUNDSTREAMBLOCK
- ST_DEFINEBITSLOSSLESS2
- ST_DEFINESHAPE3
- ST_PLACEOBJECT2
- ST_REMOVEOBJECT2
- ST_SHOWFRAME
- ST_END



其中除了ST_FILEATTRIBUTES以外的Tag和SWF含义相同，具体请参考SWF文件格式规范。

### 动态Tag ID
为了增加可能的破解难度，

### ST_TITLE 
ST_TITLE是开始的第一个Tag，它记录了本视频的标题，考虑汉字在UTF8中占据3个字节，在UTF16中占据2个字节，故标题采用UTF16编码方式。

### ST_KEYWORD
ST_KEYWORD是第二个Tag，它记录了供搜索时的关键字，关键字采用UTF16编码，关键字中间用“｜”分割。

### ST_DEFINESHAPE2

- U16 : ShapeId
- U16 : xmin
- U16 : xmax
- U16 : ymin
- U16 : ymax
- U8 : Items. Values from 1 to 4
- U16 : CharacterID
- U16 : dx
- U16 : dy


