class CXCamera
{
private:
protected:
	//Stores the position and three vectors
	D3DXVECTOR3 m_Position;
	D3DXVECTOR3 m_LookAt;
	D3DXVECTOR3 m_Right;
	D3DXVECTOR3 m_Up;
	
	//Stores the rotation to apply to a given axis
	float m_fRotAboutUp;
    float m_fRotAboutRight;
    float m_fRotAboutFacing;

	//Final view transformation matrix
	D3DXMATRIX m_ViewTransform;

	LPDIRECT3DDEVICE9 m_pDevice;

	//Stores whether the camera has changed since last update.
    //By change, we mean whether the camera has moved
   //or rotated. If so, we�ll recompute the matrix.
	bool m_UpdateRequired;

	//Called each frame to update the total view matrix
	HRESULT UpdateCameraMatrices();
public:
	void LookAtPos(D3DXVECTOR3 *Position, D3DXVECTOR3 *LookAt, D3DXVECTOR3 *Up);
	void SetPosition(FLOAT X, FLOAT Y, FLOAT Z);
	D3DXVECTOR3* GetPosition() {return &m_Position;}
	D3DXVECTOR3* GetLookAt() {return &m_LookAt;}
	D3DXVECTOR3* GetRight() {return &m_Right;}
	D3DXVECTOR3* GetUp() {return &m_Up;}
	D3DXMATRIX* GetViewMatrix() {return &m_ViewTransform;}
	CXCamera(LPDIRECT3DDEVICE9 pDevice);
	HRESULT Update();

	void RotateDown(float fAngle);
	void RotateRight(float fAngle);
	void Roll(float fAngle);
	void MoveForward(float fDist);
	void MoveRight(float fDist);
	void MoveUp(float fDist);
	void MoveInDirection(float fDist, D3DXVECTOR3* Dir);
};

//---------------------------------------------------------------------------------

CXCamera::CXCamera(LPDIRECT3DDEVICE9 pDevice)
{
	m_Position = D3DXVECTOR3(0.0f,0.0f,0.0f);
	m_LookAt = D3DXVECTOR3(0.0f,0.0f,1.0f);
	m_Right = D3DXVECTOR3(1.0f,0.0f,0.0f);
	m_Up = D3DXVECTOR3(0.0f,1.0f,0.0f);
	m_UpdateRequired = false;

	m_fRotAboutUp = m_fRotAboutRight = m_fRotAboutFacing = 0.0f;
	D3DXMatrixIdentity(&m_ViewTransform);

	m_pDevice = pDevice;
}

HRESULT CXCamera::Update()
{
	if(m_pDevice)
	{
		//Matrices to store the transformations about our axes
		if(m_UpdateRequired)
			return UpdateCameraMatrices();

		return m_pDevice->SetTransform(D3DTS_VIEW, &m_ViewTransform);
	}

	return E_FAIL;
}

HRESULT CXCamera::UpdateCameraMatrices()
{
	//Matrices to store the transformations about our axes
	D3DXMATRIX matTotal;
	D3DXMATRIX matRotAboutUp, matRotAboutRight, matRotAboutFacing;

	//Get the matrix for each rotation
	//once for each axis (up,right, and look at)
	D3DXMatrixRotationAxis(&matRotAboutRight,&m_Right,m_fRotAboutRight);
    D3DXMatrixRotationAxis(&matRotAboutUp,&m_Up,m_fRotAboutUp);
    D3DXMatrixRotationAxis(&matRotAboutFacing,&m_LookAt,m_fRotAboutFacing);
	
	//Combine the transformations into one matrix
	D3DXMatrixMultiply(&matTotal,&matRotAboutUp,&matRotAboutRight);
    D3DXMatrixMultiply(&matTotal,&matRotAboutFacing,&matTotal);

	//Transforms two vectors by our matrix and computes the third by
    //cross product
	D3DXVec3TransformCoord(&m_Right,&m_Right,&matTotal);   
    D3DXVec3TransformCoord(&m_Up,&m_Up,&matTotal);   
    D3DXVec3Cross(&m_LookAt,&m_Right,&m_Up);

	//Check to ensure vectors are perpendicular
	if(fabs(D3DXVec3Dot(&m_Up,&m_Right)) > 0.01)
	{      
		//If they�re not
		D3DXVec3Cross(&m_Up,&m_LookAt,&m_Right);
    }

	//Normalize our vectors
	D3DXVec3Normalize(&m_Right,&m_Right);
    D3DXVec3Normalize(&m_Up,&m_Up);
    D3DXVec3Normalize(&m_LookAt,&m_LookAt);

	//Compute the bottom row of the view matrix
	float fView41,fView42,fView43;
    fView41 = -D3DXVec3Dot(&m_Right,&m_Position);
    fView42 = -D3DXVec3Dot(&m_Up,&m_Position);
    fView43 = -D3DXVec3Dot(&m_LookAt,&m_Position);

	//Fill in the view matrix
	// Here's what the final view matrix should look like:   
//   
//  |  rx   ry   rz  -(r.e) |   
//  |  ux   uy   uz  -(u.e) |   
//  | -lx  -ly  -lz   (l.e) |   
//  |   0    0    0     1   |   
//   
// Where r = Right vector   
//       u = Up vector   
//       l = Look vector   
//       e = Eye position in world space   
//       . = Dot-product operation   
//   
	m_ViewTransform = D3DXMATRIX(m_Right.x, m_Up.x, m_LookAt.x, 0.0f,
								 m_Right.y, m_Up.y, m_LookAt.y, 0.0f,
                                 m_Right.z, m_Up.z, m_LookAt.z, 0.0f,
                                 fView41, fView42,  fView43,	1.0f);

	//Reset update members
	m_fRotAboutUp = m_fRotAboutRight = m_fRotAboutFacing = 0.0f;
	m_UpdateRequired = false;
	
	//Set view transform
	return m_pDevice->SetTransform(D3DTS_VIEW, &m_ViewTransform);
}

void CXCamera::LookAtPos(D3DXVECTOR3 *Position, D3DXVECTOR3 *LookAt, D3DXVECTOR3 *Up)
{
	D3DXMatrixLookAtLH(&m_ViewTransform, Position, LookAt, Up);

	m_Position = *(Position);

	m_Right.x = m_ViewTransform._11;
	m_Right.y = m_ViewTransform._21;
	m_Right.z = m_ViewTransform._31;

	m_Up.x = m_ViewTransform._12;
	m_Up.y = m_ViewTransform._22;
	m_Up.z = m_ViewTransform._32;

	m_LookAt.x = m_ViewTransform._13;
	m_LookAt.y = m_ViewTransform._23;
	m_LookAt.z = m_ViewTransform._33;

	m_fRotAboutUp = m_fRotAboutRight = m_fRotAboutFacing = 0.0f;
}

void CXCamera::SetPosition(FLOAT X, FLOAT Y, FLOAT Z)
{
	m_Position = D3DXVECTOR3(X, Y, Z);
	m_UpdateRequired = true;
}

void CXCamera::RotateDown(float fAngle)
{
	m_fRotAboutRight += fAngle;
	m_UpdateRequired = true;
}

void CXCamera::RotateRight(float fAngle)
{
	m_fRotAboutUp += fAngle;
	m_UpdateRequired = true;
}

void CXCamera::Roll(float fAngle)
{
	m_fRotAboutFacing += fAngle;
	m_UpdateRequired = true;
}

void CXCamera::MoveForward(float fDist)
{
	m_Position += fDist * m_LookAt;
	m_UpdateRequired = true;
}

void CXCamera::MoveRight(float fDist)
{
	m_Position += fDist * m_Right;
	m_UpdateRequired = true;
}

void CXCamera::MoveUp(float fDist)
{
	m_Position += fDist * m_Up;
	m_UpdateRequired = true;
}

void CXCamera::MoveInDirection(float fDist, D3DXVECTOR3* Dir)
{
	D3DXVECTOR3 DirToMove(0,0,0);
	D3DXVec3Normalize(&DirToMove, Dir);
	m_Position += fDist*DirToMove;
	m_UpdateRequired = true;
}
