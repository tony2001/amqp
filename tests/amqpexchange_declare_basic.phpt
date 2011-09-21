--TEST--
AMQPExchange
--SKIPIF--
<?php if (!extension_loaded("amqp")) print "skip"; ?>
--FILE--
<?php
$cnn = new AMQPConnection();
$cnn->connect();

$ex = new AMQPExchange($cnn);
echo $ex->declare('exchange-' . time()) ? 'true' : 'false';
?>
--EXPECT--
true