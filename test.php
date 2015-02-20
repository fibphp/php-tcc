<?php
$max = 10000000;
$t1=microtime(true);
for ($i = 0; $i < $max; $i++) {
    $a = 1234 + 5678 + $i;
    $b = 1234 * 5678 + $i;
    $c = 1234 / 2 + $i;
}
$total = $a+$b+$c;
$t2=microtime(true);
dl("tcc.so");
$func = "
       long test1(long max){
       	 long i = 0,a=0,b=0,c=0;
       	 for(i=0;i<max;++i){
       	 	a = 1234 + 5678 + i;
       	 	b = 1234 * 5678 + i;
    		c = 1234 / 2 + i;
       	 }
       	 return a+b+c;
       }
";
$i = new Tcc($func);
$i->func_define("int","test1","int");
//var_dump($i);
$i->compile();
//var_dump($i);
$ct = $i->call($max);
$t3=microtime(true);
echo "php {$total} cost:".($t2-$t1)."\n";
echo "c {$ct} cost:".($t3-$t2)."\n";