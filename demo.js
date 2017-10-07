
alert = print;

function test(a, b){
    print("a = ", a, ", b = ", b);
    return ("hel");
}

test(1, 2);
print("hello");

// --------------- if语法测试 ok
if( true ) {
    print("true");
} else {
    print("false");
}

if( false ) {
    print("true");
} else {
    print("false");
}

//  ---------------   while语法测试 ok
var i=1;
while(i<10) {
    print("while测试: ", i);
    i= i+1;
}


// ---------------  for语法测试 有错误!!!!!
for ( i=1; 10; 1 ) {
    print("for测试: i=", i);
}

// ---------------  json语法 测试ok
var t = {
    "appId": "R4AB842832E84BBD8B2DD6537DAFF790",
    "asr": "我要听新闻",
    "cloud": true,
    "intent": "playnews",
    "pattern": "^($iwant|$can|$forMe)?$play$today?的?$newsmode?的?$newstype?类?$keyword",
    "slots": {
        "iwant": "{\"type\":\"iwant\",\"value\":\"我要\"}",
        "keyword": "{\"type\":\"keyword\",\"value\":\"新闻\"}",
        "play": "{\"type\":\"play\",\"value\":\"听\"}"
    }
};

print(t.slots.iwant);

// ---------------  闭包 测试ok
function outerFun()
{
    var a=0;
    function innerFun()
    {
        a = a+1;
        alert(a);
    }
    return innerFun;  //注意这里
}
var obj=outerFun();
obj();  //结果为1
obj();  //结果为2
var obj2=outerFun();
obj2();  //结果为1
obj2();  //结果为2


// !!!!! ++ -- 暂不支持
var name = "The Window";
var object = {
    name : "My Object",
    getNameFunc : function(){
        return function(){
            return this.name;
        };
    }
};
alert(object.getNameFunc()());  //The Window

function box(){
    var arr=[];
    for(i=0; 5;1){
        arr[i]=function(){return i;}
    }
    return arr;
}
var a=box();
alert(a);//包含五个函数体的数组
alert(a[0]());
alert(a[1]());

