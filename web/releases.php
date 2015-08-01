<?php

echo "<font size='6px' color=#005f7f>";
echo "<center>Visteon Software Releases<br></center>";
echo "</font><br>";

echo '<p align="center"> <a href="main.php">Home Page</a></p>';

session_start();
$username=$_SESSION['username'];
$password=$_SESSION['password'];
$database=$_SESSION['database'];

mysql_connect(localhost,$username,$password);
@mysql_select_db($database) or die( "Unable to select database");


$query="select * from sotadb.swreleasetbl ORDER by no DESC";
$swreleasetbl=mysql_query($query);

$rel_rows=mysql_numrows($swreleasetbl);
$rel_cols=mysql_num_fields($swreleasetbl);

/* print vehicles - HEADER */
echo "<table border=1><tr>";
$i=0;while ($i < $rel_cols) {
	$meta = mysql_fetch_field($swreleasetbl, $i);
	echo "<th>$meta->name</th>";
	$i++;
}
echo "</tr>";

/* print vehicles - DATA */
$i=0;while ($i < $rel_rows) {
	$row=mysql_fetch_row($swreleasetbl);
	echo "<tr>";
	$j=0;while ($j < $rel_cols) {
		echo "<td>$row[$j]</td>";
		$j++;
	}
	echo "</tr>";
	$i++;
}
echo "</table>";
 
mysql_free_result($swreleasetbl);
mysql_close();

//print_r (phpinfo());
?>
