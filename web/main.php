<html>
<body>


<font size='7px' color=#ff5f00><center>Visteon SOTA<br></center></font>
<p align="right"> <a href="releases.php">Software Releases</a></p>

<form action="update_sota_id.php" method="post">
 <font face=arial size=3 color=#007f5f>
  ID: <input type="text" name="reg_id"/>
  New Version: <input type="text" name="new_version"/>
  Update Allowed: <input type="text" name="update_allowed"/>
  <input type="submit" value="update"/>
 </font>
</form>


<?php
header("Refresh:15;url=main.php");
/*************************************************************************
 * PHP CODE STARTS HERE
 */
session_start();
$username=$_SESSION['username'];
$password=$_SESSION['password'];
$database=$_SESSION['database'];

mysql_connect(localhost,$username,$password);
@mysql_select_db($database) or die( "Unable to select database");


$query="select * from sotadb.sotatbl ORDER BY id DESC";
$sotatbl=mysql_query($query);

$veh_rows=mysql_numrows($sotatbl);
$veh_cols=mysql_num_fields($sotatbl);


/* print vehicles - HEADER */
echo "<table border=1><tr>";
$i=0;while ($i < $veh_cols) {
	$meta = mysql_fetch_field($sotatbl, $i);
	echo "<th>$meta->name</th>";
	$i++;
}
echo "</tr>";

/* print vehicles - DATA */
$i=0;while ($i < $veh_rows) {
	$row=mysql_fetch_row($sotatbl);
	echo "<tr>";
	if($i & 1)
		$bgc = "#ffffff";
	else
		$bgc = "#dfefff";

	$j=0;while ($j < $veh_cols) {
		echo "<td bgcolor=$bgc ><font face=tahoma size=2>$row[$j]</font></td>";
		$j++;
	}
	echo "</tr>";
	$i++;
}
echo "</table>";
 
mysql_free_result($sotatbl);
mysql_close();

//print_r (phpinfo());
?>

</body>
</html>
