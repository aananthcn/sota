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

$link = mysqli_connect(localhost,$username,$password, $database);


/* update download condition */
$query="UPDATE sotadb.$ecu_table SET new_version='$new_version' WHERE ecu_name='$ecu_name'";
echo "$query <br>";
$result=mysqli_query($link, $query) or die(mysqli_error());
echo "Success";

mysqli_free_result($result);
mysqli_close();
header('Location: ' . $_SERVER['HTTP_REFERER']);

?>
