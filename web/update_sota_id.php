<?php
/**************************************************************************
 * PHP CODE STARTS HERE
 */
session_start();
$username=$_SESSION['username'];
$password=$_SESSION['password'];
$database=$_SESSION['database'];
$reg_id="0";$update_allowed="0";


if (isset($_POST['reg_id']))   {
	$reg_id=$_POST['reg_id'];
}
if (isset($_POST['update_allowed']))   {
	$update_allowed=$_POST['update_allowed'];
}


if(0 == strcmp($reg_id,"0")) {
	echo "Check your inputs!!";
	header('Location: ' . $_SERVER['HTTP_REFERER']);
}

$link = mysqli_connect(localhost,$username,$password, $database);

/* check if the id is valid */
$query1="SELECT vin FROM sotadb.sotatbl WHERE id='$reg_id'";
echo "$query1 <br>";
$result=mysqli_query($link, $query1) or die(mysqli_error());
$num=mysqli_num_rows($result);
if($num != 1) {
	echo "Query failed!!";
	header('Location: ' . $_SERVER['HTTP_REFERER']);
}

/* update download condition */
$query2="UPDATE sotadb.sotatbl SET allowed='$update_allowed' WHERE id='$reg_id'";
echo "$query2 <br>";
$result=mysqli_query($link, $query2) or die(mysqli_error());
echo "Success";

mysqli_free_result($result);
mysqli_close();
header('Location: ' . $_SERVER['HTTP_REFERER']);

?>
