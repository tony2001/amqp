--TEST--
Connection Exception
--SKIPIF--
<?php if (!extension_loaded("amqp")) print "skip"; ?>
--FILE--
<?php
$lServer['host'] = 'ip.ad.dr.ess';
$lServer['port'] = '5672';
$lServer['vhost'] = '/test';
$lServer['user'] = 'test';
$lServer['password'] = 'test';
try {
    $conn = new AMQPConnection( $lServer );
    echo "No exception thrown\n";
} catch (Exception $e) {
    echo "Exception message: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
No exception thrown
