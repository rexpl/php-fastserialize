--TEST--
Check if fastserialize is loaded
--EXTENSIONS--
fastserialize
--FILE--
<?php
echo 'The extension "fastserialize" is available';
?>
--EXPECT--
The extension "fastserialize" is available
