--TEST--
Connection Exception
--SKIPIF--
<?php if (!extension_loaded("amqp")) print "skip"; ?>
--FILE--
<?php
$conn = new AMQPConnection();
$exchangeName = "bug_unknown_" . time();

if (!$conn->connect()) {
    echo "Cannot connect to the broker \n";
}

$publisher = new AMQPExchange($conn);
$publisher->declare($exchangeName, AMQP_EX_TYPE_DIRECT, AMQP_DURABLE);

$message = md5("Time: " . rand(0,time()));
$key = "routing.key";
$params = 0;
$attributes = array('delivery_mode' => AMQP_DURABLE);

$consumer = new AMQPQueue($conn);
$consumer->declare($exchangeName, AMQP_DURABLE);
$consumer->bind($exchangeName, $key);

for ($i = 0; $i < 10; $i++ ) {
    $published = $publisher->publish($message, $key, $params, $attributes);
    if (!$published) {
        echo "message publishing failed\n";
    }
}

$options = array(
    'min' => 1,
    'max' => 5,
    'ack' => true
);
$data = $consumer->consume($options);
echo "Success\n";
?>
--EXPECT--
Success
