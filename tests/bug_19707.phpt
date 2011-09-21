--TEST--
AMQPQueue::get() doesn't return the message
--SKIPIF--
<?php if (!extension_loaded("amqp")) print "skip"; ?>
--FILE--
<?php
$cnn = new AMQPConnection();
$cnn->connect();
$ex = new AMQPExchange($cnn);
$ex->declare('exchange11', AMQP_EX_TYPE_FANOUT);
$q = new AMQPQueue($cnn, 'queue5');
$q->declare();
$ex->bind('queue5', 'routing.key');
$ex->publish('message', 'routing.key');
$msg = $q->get();
echo "message received from get: " . print_r($msg, true) . "\n";
?>
--EXPECTF--
message received from get: Array
(
    [routing_key] => routing.key
    [exchange] => exchange11
    [delivery_tag] => 1
    [Content-type] => text/plain
    [count] => 0
    [msg] => message
)
