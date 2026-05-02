local Cube = {}

function Cube:OnCreate()
	 print("Cube created")
end

function Cube:OnUpdate(dt)
    local transform = self.entity:GetTransform()

    if IsKeyDown(KEY_W) then
        transform.Translation.z = transform.Translation.z - 5.0 * dt
    end

    if IsKeyDown(KEY_S) then
        transform.Translation.z = transform.Translation.z + 5.0 * dt
    end

    if IsKeyDown(KEY_A) then
        transform.Translation.x = transform.Translation.x - 5.0 * dt
    end

    if IsKeyDown(KEY_D) then
        transform.Translation.x = transform.Translation.x + 5.0 * dt
    end
end

return Cube