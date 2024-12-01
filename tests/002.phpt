--TEST--
test1() Basic test
--EXTENSIONS--
fastserialize
--FILE--
<?php
$ret = test1();

var_dump($ret);
?>
--EXPECT--
The extension fastserialize is loaded and working!
NULL
