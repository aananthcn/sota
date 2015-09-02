<?php
/**************************************************************************
 * PHP CODE STARTS HERE
 */
session_start();
$username=$_SESSION['username'];
$password=$_SESSION['password'];
$database=$_SESSION['database'];
$ecu_table=$_SESSION['ecu_table'];
$ecu_name="0";$new_version="0";


if (isset($_POST['ecu_name']))   {
	$ecu_name=$_POST['ecu_name'];
}
if (isset($_POST['new_version']))   {
	$new_version=$_POST['new_version'];
}

if(0 == strcmp($ecu_name,"0")) {
	echo "Check your inputs!!";
	header('Location: ' . $_SERVER['HTTP_REFERER']);
}

mysql_connect(localhost,$username,$password);
@mysql_select_db($database) or die( "Unable to select database");


/* update download condition */
$query="UPDATE sotadb.$ecu_table SET new_version='$new_version' WHERE ecu_name='$ecu_name'";
echo "$query <br>";
$result=mysql_query($query) or die(mysql_error());
echo "Success";

mysql_free_result($result);
mysql_close();
header('Location: ' . $_SERVER['HTTP_REFERER']);

?>
